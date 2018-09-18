CREATE DATABASE IF NOT EXISTS Barista;

USE Barista;

DROP TABLE IF EXISTS switches;
CREATE TABLE switches ( \
	dpid         BIGINT UNSIGNED NOT NULL, \
	num_tables   INT    UNSIGNED,          \
	num_buffers  INT    UNSIGNED,          \
	capabilities INT    UNSIGNED,          \
	actions      INT    UNSIGNED,          \
        mfr_desc     VARCHAR(256),             \
        hw_desc      VARCHAR(256),             \
        serial_num   VARCHAR(32),              \
        dp_desc      VARCHAR(256),             \
	ctrl_ip      VARCHAR(15)     NOT NULL, \
	PRIMARY KEY(dpid)                      \
);

DROP TABLE IF EXISTS hosts;
CREATE TABLE hosts ( \
	dpid         BIGINT UNSIGNED NOT NULL, \
	port         INT    UNSIGNED NOT NULL, \
	ip           VARCHAR(15)     NOT NULL, \
	mac          VARCHAR(17)     NOT NULL, \
        ctrl_ip      VARCHAR(15)     NOT NULL, \
	PRIMARY KEY(ip)                        \
);

DROP TABLE IF EXISTS links;
CREATE TABLE links ( \
	src_dpid     BIGINT UNSIGNED NOT NULL, \
	src_port     INT    UNSIGNED NOT NULL, \
	dst_dpid     BIGINT UNSIGNED NOT NULL, \
	dst_port     INT    UNSIGNED NOT NULL, \
        rx_packets   BIGINT UNSIGNED,          \
        rx_bytes     BIGINT UNSIGNED,          \
        tx_packets   BIGINT UNSIGNED,          \
        tx_bytes     BIGINT UNSIGNED,          \
        ctrl_ip      VARCHAR(15)     NOT NULL, \
	PRIMARY KEY(src_dpid, src_port)        \
);

DROP TABLE IF EXISTS flows;
CREATE TABLE flows ( \
        time         DATETIME        DEFAULT CURRENT_TIMESTAMP, \
	dpid         BIGINT UNSIGNED NOT NULL, \
        port         INT    UNSIGNED NOT NULL, \
        idle_timeout INT    UNSIGNED NOT NULL, \
        hard_timeout INT    UNSIGNED NOT NULL, \
        wildcards    INT    UNSIGNED NOT NULL, \
        vlan_id      INT    UNSIGNED NOT NULL, \
        vlan_pcp     INT    UNSIGNED NOT NULL, \
        proto        VARCHAR(8)      NOT NULL, \
        ip_tos       INT    UNSIGNED NOT NULL, \
        src_mac      VARCHAR(17)     NOT NULL, \
        dst_mac      VARCHAR(17)     NOT NULL, \
        src_ip       VARCHAR(15)     NOT NULL, \
        dst_ip       VARCHAR(15)     NOT NULL, \
        src_port     INT    UNSIGNED NOT NULL, \
        dst_port     INT    UNSIGNED NOT NULL, \
        actions      VARCHAR(128)    NOT NULL, \
	ctrl_ip      VARCHAR(15)     NOT NULL, \
        pkt_count    BIGINT UNSIGNED,          \
        byte_count   BIGINT UNSIGNED           \
);

DROP TABLE IF EXISTS traffic;
CREATE TABLE traffic ( \
	id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, \
        time         DATETIME        DEFAULT CURRENT_TIMESTAMP, \
	in_pkt_cnt   BIGINT UNSIGNED NOT NULL, \
	in_byte_cnt  BIGINT UNSIGNED NOT NULL, \
	out_pkt_cnt  BIGINT UNSIGNED NOT NULL, \
	out_byte_cnt BIGINT UNSIGNED NOT NULL, \
        ctrl_ip      VARCHAR(15)     NOT NULL, \
	PRIMARY KEY(id)                        \
);

DROP TABLE IF EXISTS resources;
CREATE TABLE resources ( \
	id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, \
        time         DATETIME        DEFAULT CURRENT_TIMESTAMP, \
        cpu          DOUBLE          NOT NULL, \
        mem          DOUBLE          NOT NULL, \
        ctrl_ip      VARCHAR(15)     NOT NULL, \
        PRIMARY KEY(id)                        \
);

DROP TABLE IF EXISTS logs;
CREATE TABLE logs ( \
	id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, \
	time         DATETIME        DEFAULT CURRENT_TIMESTAMP, \
	log          VARCHAR(2048)   NOT NULL, \
	ctrl_ip      VARCHAR(15)     NOT NULL, \
	PRIMARY KEY(id)                        \
);

DROP TABLE IF EXISTS events;
CREATE TABLE events ( \
	id           BIGINT UNSIGNED NOT NULL AUTO_INCREMENT, \
        event        INT UNSIGNED    NOT NULL, \
	data         VARCHAR(2048)   NOT NULL, \
	ctrl_ip      VARCHAR(15)     NOT NULL, \
        PRIMARY KEY(id)                        \
);

DROP TABLE IF EXISTS clusters;
CREATE TABLE clusters ( \
        cluster_id   BIGINT UNSIGNED NOT NULL, \
	ctrl_ip      VARCHAR(15)     NOT NULL, \
	time         DATETIME        DEFAULT CURRENT_TIMESTAMP, \
	PRIMARY KEY(ctrl_ip)                   \
);

CREATE DATABASE IF NOT EXISTS Barista_Mgmt;

USE Barista_Mgmt;

DROP TABLE IF EXISTS users;
CREATE TABLE users ( \
	user_id      VARCHAR(32)     NOT NULL, \
	user_pw      VARCHAR(32)     NOT NULL, \
	PRIMARY KEY(user_id)                   \
);

INSERT INTO Barista_Mgmt.users (user_id, user_pw) values ('barista', 'barista');
