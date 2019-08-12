## pg_listen: trigger a shell command on a Postgres event

Super fast and lightweight. Written in C using libpq.

### Usage

```bash
pg_listen postgres://db-uri channel /path/to/program [args]

# for example, to note when NOTIFY happened on "hello" channel
# pg_listen postgres://localhost/postgres hello /bin/echo they said hi

# or to output the payload from the "fun" channel
# pg_listen postgres://localhost/postgres fun /bin/cat -
```

### Building

Just clone the repo and run `make`. The makefile is compatible with BSD and GNU Make and requires only that libpq be installed on the system.

If you use NixOS you can run `nix-shell` to build it.

If you installed PostgreSQL on Mac using homebrew, note that pkg-config is not installed by default and needs to be installed by running `brew install pkg-config` prior to running `make`.