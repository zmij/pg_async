drop schema if exists lobby cascade;

create schema lobby;

------------------------------------------------------------------------------
--	auth.sessions
------------------------------------------------------------------------------
create table if not exists auth.sessions (
	sid			uuid		not null default gen_random_uuid(),
	ctime		timestamptz	not null default current_timestamp,
	mtime		timestamptz	not null default current_timestamp,
	uid			uuid		not null,
	ip			inet,
	constraint sessions_user_key foreign key (uid) 
		references auth.users(uid) on delete cascade on update cascade,
	constraint sessions_pkey primary key (sid) 
);

