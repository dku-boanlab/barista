CREATE DATABASE IF NOT EXISTS l2_learning;

USE l2_learning;

DROP TABLE IF EXISTS forwarding_table;
CREATE TABLE forwarding_table
(
    DPID		BIGINT UNSIGNED NOT NULL,
    PORT		INT UNSIGNED NOT NULL,
    MAC			BIGINT UNSIGNED NOT NULL,
    IP			INT UNSIGNED NOT NULL,
    INSTANCE		VARCHAR(32) NOT NULL,
    PRIMARY KEY(DPID, MAC)
);