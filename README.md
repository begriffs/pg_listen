## pg_listen: trigger a shell command on a Postgres event

Super fast and lightweight. Written in C using libpq.

### Usage

```bash
pg_listen postgres://db-uri channel [/path/to/program] [args]

# for example, to note when NOTIFY happened on "hello" channel
# pg_listen postgres://localhost/postgres hello /bin/echo they said hi

# print payload from the channel
# (default action when no command is specified)
# pg_listen postgres://localhost/postgres fun
```

Note that pg\_listen line-buffers its output, so the payload raised by NOTIFY
needs to include a final newline ("\n"). The program won't output anything
until a newline is encountered.

```sql
-- incorrect
NOTIFY foo, 'hi';

-- correct
NOTIFY foo, E'hi\n';
```

### Building

Requirements:

* PostgreSQL and
  [libpq](https://www.postgresql.org/docs/current/libpq-build.html).
* [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
* `$PKG_CONFIG_PATH` including the directory containing `libpq.pc`
* C89 and POSIX

Just clone the repo and run:

```sh
./configure
make
```
