/*------------------------------------------------------------------------
 *
 * xlogarchive.h
 *		Prototypes for WAL archives in the backend
 *
 * Portions Copyright (c) 1996-2023, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *		src/include/access/xlogarchive.h
 *
 *------------------------------------------------------------------------
 */

#ifndef XLOG_ARCHIVE_H
#define XLOG_ARCHIVE_H

#include "access/xlogdefs.h"

extern bool RestoreArchivedFile(char *path, const char *xlogfname,
								const char *recovername, off_t expectedSize,
								bool cleanupEnabled);
extern void KeepFileRestoredFromArchive(const char *path, const char *xlogfname);
extern void XLogArchiveNotify(const char *xlog);
extern void XLogArchiveNotifySeg(XLogSegNo segno, TimeLineID tli);
extern void XLogArchiveForceDone(const char *xlog);
extern bool XLogArchiveCheckDone(const char *xlog);
extern bool XLogArchiveIsBusy(const char *xlog);
extern bool XLogArchiveIsReady(const char *xlog);
extern bool XLogArchiveIsReadyOrDone(const char *xlog);
extern void XLogArchiveCleanup(const char *xlog);

extern bool shell_restore(const char *file, const char *path,
						  const char *lastRestartPointFileName);
extern void shell_archive_cleanup(const char *lastRestartPointFileName);
extern void shell_recovery_end(const char *lastRestartPointFileName);

#endif							/* XLOG_ARCHIVE_H */
