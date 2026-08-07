PRAGMA foreign_keys = false;

DROP TABLE IF EXISTS "map";
CREATE TABLE "map" (
  "Value" TEXT(255),
  "Key" TEXT(255)
);

DROP TABLE IF EXISTS "map_ish";
CREATE TABLE "map_ish" (
  "Value" TEXT(255),
  "Key" TEXT(255)
);

DROP TABLE IF EXISTS "map_vh";
CREATE TABLE "map_vh" (
  "Value" TEXT(255),
  "Key" TEXT(255)
);

DROP TABLE IF EXISTS "ports";
CREATE TABLE "ports" (
  "port" integer NOT NULL,
  "god" VARCHAR NOT NULL,
  PRIMARY KEY ("port", "god")
);

DROP TABLE IF EXISTS "settings";
CREATE TABLE "settings" (
  "key" VARCHAR NOT NULL,
  "value" VARCHAR,
  PRIMARY KEY ("key")
);

PRAGMA foreign_keys = true;
