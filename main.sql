/*
 Navicat Premium Data Transfer

 Source Server         : res2
 Source Server Type    : SQLite
 Source Server Version : 3035005 (3.35.5)
 Source Schema         : main

 Target Server Type    : SQLite
 Target Server Version : 3035005 (3.35.5)
 File Encoding         : 65001

 Date: 31/05/2026 17:26:13
*/

PRAGMA foreign_keys = false;

-- ----------------------------
-- Table structure for map
-- ----------------------------
DROP TABLE IF EXISTS "map";
CREATE TABLE "map" (
  "Value" TEXT(255),
  "Key" TEXT(255)
);

-- ----------------------------
-- Table structure for map_ish
-- ----------------------------
DROP TABLE IF EXISTS "map_ish";
CREATE TABLE "map_ish" (
  "Value" TEXT(255),
  "Key" TEXT(255)
);

-- ----------------------------
-- Table structure for map_vh
-- ----------------------------
DROP TABLE IF EXISTS "map_vh";
CREATE TABLE "map_vh" (
  "Value" TEXT(255),
  "Key" TEXT(255)
);

-- ----------------------------
-- Table structure for ports
-- ----------------------------
DROP TABLE IF EXISTS "ports";
CREATE TABLE "ports" (
  "port" integer NOT NULL,
  "god" VARCHAR NOT NULL,
  PRIMARY KEY ("port", "god")
);

-- ----------------------------
-- Table structure for res
-- ----------------------------
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
  "blank" VARCHAR
);

-- ----------------------------
-- Table structure for settings
-- ----------------------------
DROP TABLE IF EXISTS "settings";
CREATE TABLE "settings" (
  "key" VARCHAR NOT NULL,
  "value" VARCHAR,
  PRIMARY KEY ("key")
);

PRAGMA foreign_keys = true;
