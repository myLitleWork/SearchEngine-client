PRAGMA foreign_keys = false;

DROP TABLE IF EXISTS "res";
CREATE TABLE "res" (
  "num" INTEGER NOT NULL,
  "date" DATE,
  "date_podp" DATE,
  "isp" VARCHAR,
  "type" VARCHAR,
  "podp_num" VARCHAR,
  "from_to" VARCHAR,
  "rel" real,
  "dir" VARCHAR,
  "tel_num" INTEGER,
  "kr" VARCHAR,
  "pril_name" VARCHAR,
  "pril_count" VARCHAR,
  "last_mesto" VARCHAR,
  "gde_sht" VARCHAR,
  "blank" VARCHAR,
  "deleted" INTEGER DEFAULT 0
);

PRAGMA foreign_keys = true;
