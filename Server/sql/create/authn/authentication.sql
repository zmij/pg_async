drop schema if exists auth cascade;

create schema auth;

create type auth.method as enum ( 
	'token',
	'apple_vendor_uid',
	'password', 
	'apple_id', 
	'google_id', 
	'facebook',
	'oauth'
);

------------------------------------------------------------------------------
--	auth.users
------------------------------------------------------------------------------
create table if not exists auth.users (
	uid			uuid		not null default gen_random_uuid(),
	ctime		timestamptz	not null default current_timestamp,
	mtime		timestamptz	not null default current_timestamp,
	name		text		not null,
	online		bool		default false,
	locale		text,
	timezone	text,
	constraint users_pkey primary key (uid) 
);

------------------------------------------------------------------------------
--	auth.authn basic table for authentication methods
------------------------------------------------------------------------------
create table if not exists auth.authn (
	uid			uuid		not null,
	ctime		timestamptz	not null default current_timestamp,
	mtime		timestamptz	not null default current_timestamp,
	method		auth.method	not null default 'password',
	constraint authn_pkey primary key (uid, method),
	constraint authn_user_key foreign key (uid) 
		references auth.users(uid) on delete cascade on update cascade
);

create trigger authn_update_mtime 
before update on auth.authn
for each row
execute procedure core.update_mtime();

------------------------------------------------------------------------------
--	auth.tokens
------------------------------------------------------------------------------
create table if not exists auth.tokens (
	token		uuid		not null default gen_random_uuid(),
	constraint tokens_pkey primary key(token, uid),
	constraint tokens_method_check check (method = 'token'),
	constraint tokens_user_key foreign key (uid) 
		references auth.users(uid) on delete cascade on update cascade
) inherits (auth.authn);

create trigger tokens_update_mtime 
before update on auth.tokens
for each row
execute procedure core.update_mtime();

------------------------------------------------------------------------------
--	auth.apple_vendor_uids
------------------------------------------------------------------------------
create table if not exists auth.apple_vendor_uids (
	apple_uid	uuid		not null,
	constraint apple_vendor_uids_pkey primary key(apple_uid),
	constraint apple_vendor_uids_method_check check (method = 'apple_vendor_uid'),
	constraint apple_vendor_uids_user_key foreign key (uid) 
		references auth.users(uid) on delete cascade on update cascade
) inherits (auth.authn);

create trigger apple_vendor_uids_update_mtime 
before update on auth.apple_vendor_uids
for each row
execute procedure core.update_mtime();

------------------------------------------------------------------------------
--	auth.passwords
------------------------------------------------------------------------------
create table if not exists auth.passwords (
	login		text		not null,
	pwd_hash	bytea		not null,	-- SHA256 hash of password
	constraint password_pkey primary key (uid),
	constraint password_unique_login unique (login),
	constraint password_type_check check (method = 'password'),
	constraint password_user_key foreign key (uid) 
		references auth.users(uid) on delete cascade on update cascade
) inherits (auth.authn);

create trigger passwords_update_mtime 
before update on auth.passwords
for each row
execute procedure core.update_mtime();

create or replace view auth.user_passwords 
as select u.uid, u.name, p.login, p.pwd_hash::text as pwd
from auth.users u join auth.passwords p on (p.uid = u.uid);

-- Helper rule to insert plaintext passwords and make hash
create or replace rule user_password_insert as on insert
to auth.user_passwords
do instead
insert into auth.passwords(uid, login, pwd_hash) 
values (new.uid, new.login, digest(new.pwd, 'sha256'));

-- Helper rule to update plaintext password
create or replace rule user_password_update as on update
to auth.user_passwords
do instead
update auth.passwords
	set login = new.login,
		pwd_hash = digest(new.pwd, 'sha256')
	where uid = old.uid;

