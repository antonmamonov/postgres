drop table ͪߩѦ���;
create table ͪߩѦ��� (��� text, ��׾�ڵ� varchar, ���1A�� char(16));
create index ͪߩѦ���index1 on ͪߩѦ��� using btree (���);
create index ͪߩѦ���index2 on ͪߩѦ��� using hash (��׾�ڵ�);
insert into ͪߩѦ��� values('��ǻ�͵��÷���', 'ѦA01߾');
insert into ͪߩѦ��� values('��ǻ�ͱ׷��Ƚ�', '��B10��');
insert into ͪߩѦ��� values('��ǻ�����α׷���', '��Z01��');
vacuum ͪߩѦ���;
select * from ͪߩѦ���;
select * from ͪߩѦ��� where ��׾�ڵ� = '��Z01��';
select * from ͪߩѦ��� where ��׾�ڵ� ~* '��z01��';
select * from ͪߩѦ��� where ��׾�ڵ� like '_Z01_';
select * from ͪߩѦ��� where ��׾�ڵ� like '_Z%';
select * from ͪߩѦ��� where ��� ~ '��ǻ��[���]';
select * from ͪߩѦ��� where ��� ~* '��ǻ��[���]';
select *,character_length(���) from ͪߩѦ���;
select *,octet_length(���) from ͪߩѦ���;
select *,position('��' in ���) from ͪߩѦ���;
select *,substring(��� from 3 for 4) from ͪߩѦ���;