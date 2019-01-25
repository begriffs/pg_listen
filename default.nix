with import <nixpkgs>{};
stdenv.mkDerivation {
  name = "pg_listen";
  buildInputs = [ clang pkgconfig libpqxx ];
  shellHook = "make && exit";
}
