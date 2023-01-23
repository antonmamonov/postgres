/*-------------------------------------------------------------------------
 *
 * combocid.c
 *	  Combo command ID support routines
 *
 * Before version 8.3, HeapTupleHeaderData had separate fields for cmin
 * and cmax.  To reduce the header size, cmin and cmax are now overlayed
 * in the same field in the header.  That usually works because you rarely
 * insert and delete a tuple in the same transaction, and we don't need
 * either field to remain valid after the originating transaction exits.
 * To make it work when the inserting transaction does delete the tuple,
 * we create a "combo" command ID and store that in the tuple header
 * instead of cmin and cmax. The combo command ID can be mapped to the
 * real cmin and cmax using a backend-private array, which is managed by
 * this module.
 *
 * To allow reusing existing combo CIDs, we also keep a hash table that
 * maps cmin,cmax pairs to combo CIDs.  This keeps the data structure size
 * reasonable in most cases, since the number of unique pairs used by any
 * one transaction is likely to be small.
 *
 * With a 32-bit combo command id we can represent 2^32 distinct cmin,cmax
 * combinations. In the most perverse case where each command deletes a tuple
 * generated by every previous command, the number of combo command ids
 * required for N commands is N*(N+1)/2.  That means that in the worst case,
 * that's enough for 92682 commands.  In practice, you'll run out of memory
 * and/or disk space way before you reach that limit.
 *
 * The array and hash table are kept in TopTransactionContext, and are
 * destroyed at the end of each transaction.
 *
 *
 * Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/time/combocid.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "storage/shmem.h"
#include "utils/combocid.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"

/* Hash table to lookup combo CIDs by cmin and cmax */
static HTAB *comboHash = NULL;

/* Key and entry structures for the hash table */
typedef struct
{
	CommandId	cmin;
	CommandId	cmax;
} ComboCidKeyData;

typedef ComboCidKeyData *ComboCidKey;

typedef struct
{
	ComboCidKeyData key;
	CommandId	combocid;
} ComboCidEntryData;

typedef ComboCidEntryData *ComboCidEntry;

/* Initial size of the hash table */
#define CCID_HASH_SIZE			100


/*
 * An array of cmin,cmax pairs, indexed by combo command id.
 * To convert a combo CID to cmin and cmax, you do a simple array lookup.
 */
static ComboCidKey comboCids = NULL;
static int	usedComboCids = 0;	/* number of elements in comboCids */
static int	sizeComboCids = 0;	/* allocated size of array */

/* Initial size of the array */
#define CCID_ARRAY_SIZE			100


/* prototypes for internal functions */
static CommandId GetComboCommandId(CommandId cmin, CommandId cmax);
static CommandId GetRealCmin(CommandId combocid);
static CommandId GetRealCmax(CommandId combocid);


/**** External API ****/

/*
 * GetCmin and GetCmax assert that they are only called in situations where
 * they make sense, that is, can deliver a useful answer.  If you have
 * reason to examine a tuple's t_cid field from a transaction other than
 * the originating one, use HeapTupleHeaderGetRawCommandId() directly.
 */

CommandId
HeapTupleHeaderGetCmin(HeapTupleHeader tup)
{
	CommandId	cid = HeapTupleHeaderGetRawCommandId(tup);

	Assert(!(tup->t_infomask & HEAP_MOVED));
	Assert(TransactionIdIsCurrentTransactionId(HeapTupleHeaderGetXmin(tup)));

	if (tup->t_infomask & HEAP_COMBOCID)
		return GetRealCmin(cid);
	else
		return cid;
}

CommandId
HeapTupleHeaderGetCmax(HeapTupleHeader tup)
{
	CommandId	cid = HeapTupleHeaderGetRawCommandId(tup);

	Assert(!(tup->t_infomask & HEAP_MOVED));

	/*
	 * Because GetUpdateXid() performs memory allocations if xmax is a
	 * multixact we can't Assert() if we're inside a critical section. This
	 * weakens the check, but not using GetCmax() inside one would complicate
	 * things too much.
	 */
	Assert(CritSectionCount > 0 ||
		   TransactionIdIsCurrentTransactionId(HeapTupleHeaderGetUpdateXid(tup)));

	if (tup->t_infomask & HEAP_COMBOCID)
		return GetRealCmax(cid);
	else
		return cid;
}

/*
 * Given a tuple we are about to delete, determine the correct value to store
 * into its t_cid field.
 *
 * If we don't need a combo CID, *cmax is unchanged and *iscombo is set to
 * false.  If we do need one, *cmax is replaced by a combo CID and *iscombo
 * is set to true.
 *
 * The reason this is separate from the actual HeapTupleHeaderSetCmax()
 * operation is that this could fail due to out-of-memory conditions.  Hence
 * we need to do this before entering the critical section that actually
 * changes the tuple in shared buffers.
 */
void
HeapTupleHeaderAdjustCmax(HeapTupleHeader tup,
						  CommandId *cmax,
						  bool *iscombo)
{
	/*
	 * If we're marking a tuple deleted that was inserted by (any
	 * subtransaction of) our transaction, we need to use a combo command id.
	 * Test for HeapTupleHeaderXminCommitted() first, because it's cheaper
	 * than a TransactionIdIsCurrentTransactionId call.
	 */
	if (!HeapTupleHeaderXminCommitted(tup) &&
		TransactionIdIsCurrentTransactionId(HeapTupleHeaderGetRawXmin(tup)))
	{
		CommandId	cmin = HeapTupleHeaderGetCmin(tup);

		*cmax = GetComboCommandId(cmin, *cmax);
		*iscombo = true;
	}
	else
	{
		*iscombo = false;
	}
}

/*
 * Combo command ids are only interesting to the inserting and deleting
 * transaction, so we can forget about them at the end of transaction.
 */
void
AtEOXact_ComboCid(void)
{
	/*
	 * Don't bother to pfree. These are allocated in TopTransactionContext, so
	 * they're going to go away at the end of transaction anyway.
	 */
	comboHash = NULL;

	comboCids = NULL;
	usedComboCids = 0;
	sizeComboCids = 0;
}


/**** Internal routines ****/

/*
 * Get a combo command id that maps to cmin and cmax.
 *
 * We try to reuse old combo command ids when possible.
 */
static CommandId
GetComboCommandId(CommandId cmin, CommandId cmax)
{
	CommandId	combocid;
	ComboCidKeyData key;
	ComboCidEntry entry;
	bool		found;

	/*
	 * Create the hash table and array the first time we need to use combo
	 * cids in the transaction.
	 */
	if (comboHash == NULL)
	{
		HASHCTL		hash_ctl;

		/* Make array first; existence of hash table asserts array exists */
		comboCids = (ComboCidKeyData *)
			MemoryContextAlloc(TopTransactionContext,
							   sizeof(ComboCidKeyData) * CCID_ARRAY_SIZE);
		sizeComboCids = CCID_ARRAY_SIZE;
		usedComboCids = 0;

		hash_ctl.keysize = sizeof(ComboCidKeyData);
		hash_ctl.entrysize = sizeof(ComboCidEntryData);
		hash_ctl.hcxt = TopTransactionContext;

		comboHash = hash_create("Combo CIDs",
								CCID_HASH_SIZE,
								&hash_ctl,
								HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	/*
	 * Grow the array if there's not at least one free slot.  We must do this
	 * before possibly entering a new hashtable entry, else failure to
	 * repalloc would leave a corrupt hashtable entry behind.
	 */
	if (usedComboCids >= sizeComboCids)
	{
		int			newsize = sizeComboCids * 2;

		comboCids = (ComboCidKeyData *)
			repalloc(comboCids, sizeof(ComboCidKeyData) * newsize);
		sizeComboCids = newsize;
	}

	/* Lookup or create a hash entry with the desired cmin/cmax */

	/* We assume there is no struct padding in ComboCidKeyData! */
	key.cmin = cmin;
	key.cmax = cmax;
	entry = (ComboCidEntry) hash_search(comboHash,
										(void *) &key,
										HASH_ENTER,
										&found);

	if (found)
	{
		/* Reuse an existing combo CID */
		return entry->combocid;
	}

	/* We have to create a new combo CID; we already made room in the array */
	combocid = usedComboCids;

	comboCids[combocid].cmin = cmin;
	comboCids[combocid].cmax = cmax;
	usedComboCids++;

	entry->combocid = combocid;

	return combocid;
}

static CommandId
GetRealCmin(CommandId combocid)
{
	Assert(combocid < usedComboCids);
	return comboCids[combocid].cmin;
}

static CommandId
GetRealCmax(CommandId combocid)
{
	Assert(combocid < usedComboCids);
	return comboCids[combocid].cmax;
}

/*
 * Estimate the amount of space required to serialize the current combo CID
 * state.
 */
Size
EstimateComboCIDStateSpace(void)
{
	Size		size;

	/* Add space required for saving usedComboCids */
	size = sizeof(int);

	/* Add space required for saving ComboCidKeyData */
	size = add_size(size, mul_size(sizeof(ComboCidKeyData), usedComboCids));

	return size;
}

/*
 * Serialize the combo CID state into the memory, beginning at start_address.
 * maxsize should be at least as large as the value returned by
 * EstimateComboCIDStateSpace.
 */
void
SerializeComboCIDState(Size maxsize, char *start_address)
{
	char	   *endptr;

	/* First, we store the number of currently-existing combo CIDs. */
	*(int *) start_address = usedComboCids;

	/* If maxsize is too small, throw an error. */
	endptr = start_address + sizeof(int) +
		(sizeof(ComboCidKeyData) * usedComboCids);
	if (endptr < start_address || endptr > start_address + maxsize)
		elog(ERROR, "not enough space to serialize ComboCID state");

	/* Now, copy the actual cmin/cmax pairs. */
	if (usedComboCids > 0)
		memcpy(start_address + sizeof(int), comboCids,
			   (sizeof(ComboCidKeyData) * usedComboCids));
}

/*
 * Read the combo CID state at the specified address and initialize this
 * backend with the same combo CIDs.  This is only valid in a backend that
 * currently has no combo CIDs (and only makes sense if the transaction state
 * is serialized and restored as well).
 */
void
RestoreComboCIDState(char *comboCIDstate)
{
	int			num_elements;
	ComboCidKeyData *keydata;
	int			i;
	CommandId	cid;

	Assert(!comboCids && !comboHash);

	/* First, we retrieve the number of combo CIDs that were serialized. */
	num_elements = *(int *) comboCIDstate;
	keydata = (ComboCidKeyData *) (comboCIDstate + sizeof(int));

	/* Use GetComboCommandId to restore each combo CID. */
	for (i = 0; i < num_elements; i++)
	{
		cid = GetComboCommandId(keydata[i].cmin, keydata[i].cmax);

		/* Verify that we got the expected answer. */
		if (cid != i)
			elog(ERROR, "unexpected command ID while restoring combo CIDs");
	}
}
