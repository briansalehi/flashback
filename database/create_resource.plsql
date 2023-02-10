create or replace function create_resource(
    iname varchar(200),
    idescription varchar(500) default null,
    ilink varchar(5000) default null
) returns integer as
$$
begin
    insert into resources (name, description, purchase_link, creation_date)
    values (iname, idescription, ilink, now()) returning id;
end
$$
language 'plpgsql'