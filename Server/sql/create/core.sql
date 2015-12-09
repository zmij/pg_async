drop schema if exists core cascade;

create schema core;

create or replace function core.update_mtime()
returns trigger as
$BODY$
begin
	new.ctime := old.ctime;
	new.mtime := current_timestamp;
	return new;
end
$BODY$
	language plpgsql;
