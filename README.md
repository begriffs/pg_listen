## pg_listen: trigger a shell command on a Postgres event

Super fast and lightweight. Written in C using libpq.

### Usage

```bash
pg_listen postgres://db-uri channel-name shell-command

# for example
# pg_listen postgres://localhost/postgres hello "echo they said hi"
```

### Building

Just clone the repo and run `make`. The makefile is compatible with BSD and GNU Make and requires only that libpq be installed on the system.

If you use NixOS you can run `nix-shell` to build it.
