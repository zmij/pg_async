create temporary table pg_async_test (
	id 				bigserial primary key,
	ctime			timestamp default current_timestamp,
	ctimetz			timestamptz default current_timestamp,
	some_text		text,
	fixed_text		char(25),
	var_text		varchar(25),
	float_field		float,
	json_field		json,
	bool_field		boolean,
	money_field		money
);
insert into pg_async_test(some_text, fixed_text, var_text, float_field, json_field, bool_field, money_field)
values
('text 1', 'fixed text 1', 'var text 1', 1.0, '{}', true, 1.998),
('text 2', 'fixed text 1', 'var text 1', 2.0, '{}', false, 10.5),
('text 3', 'fixed text 1', 'var text 1', 3.0, '{}', true, 10005000),
('text 4', 'fixed text 1', 'var text 1', 4.0, '{}', false, 0.25),
('text 5', 'fixed text 1', 'var text 1', 5.0, '{}', true, 15),
('text 6', 'fixed text 1', 'var text 1', 6.0, '{}', false, -19),
('text 7', 'fixed text 1', 'var text 1', 7.0, '{}', true, 87),
('text 8', 'fixed text 1', 'var text 1', 8.0, '{}', false, 888),
('text 9', 'fixed text 1', 'var text 1', 9.0, '{}', true, -0.15),
('text 10', 'fixed text 1', 'var text 1', 10.0, '{}', false, 908)
returning id, ctime, ctimetz;
select * from pg_async_test;
