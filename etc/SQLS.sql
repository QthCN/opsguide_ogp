-- 初始化
CREATE DATABASE IF NOT EXISTS ogp DEFAULT CHARACTER SET utf8;
SET NAMES 'utf8';
USE ogp;

CREATE TABLE MACHINE_APP_LIST (
    id INT NOT NULL AUTO_INCREMENT,
    ip_address VARCHAR(32) COMMENT '主机的IP地址',
    app_id INT NOT NULL COMMENT '主机上运行的APP的ID',
    version_id INT NOT NULL COMMENT '主机上运行的APP的版本号ID',
    runtime_name VARCHAR(96) NOT NULL COMMENT '运行时容器的名字',
    PRIMARY KEY(id),
    KEY(ip_address),
    KEY(app_id),
    UNIQUE(runtime_name)
);

CREATE TABLE APP_LIST (
    id INT NOT NULL AUTO_INCREMENT COMMENT 'APP ID',
    source VARCHAR(256) NOT NULL COMMENT 'APP的注册方,例如JENKINS',
    name VARCHAR(64) NOT NULL COMMENT 'APP名称,也是docker image的名称',
    description VARCHAR(2048) NOT NULL COMMENT 'APP的说明信息',
    PRIMARY KEY(id),
    UNIQUE(name)
);

CREATE TABLE APP_VERSIONS (
    id INT NOT NULL AUTO_INCREMENT COMMENT 'VERSION ID',
    app_id INT NOT NULL COMMENT 'APP的ID',
    version VARCHAR(1024) NOT NULL COMMENT 'APP版本',
    registe_time DATETIME NOT NULL COMMENT '版本注册时间',
    description VARCHAR(2048) NOT NULL COMMENT '版本说明',
    PRIMARY KEY(id),
    KEY (app_id)
);

CREATE TABLE PUBLISH_APP_HINTS (
    id INT NOT NULL AUTO_INCREMENT,
    uniq_id INT NOT NULL COMMENT '对应MACHINE_APP_LIST中的ID',
    item VARCHAR(1024) NOT NULL COMMENT 'HINT项',
    value VARCHAR(1024) NOT NULL COMMENT 'HINT值',
    PRIMARY KEY (id),
    KEY (uniq_id)
);

CREATE TABLE PUBLISH_APP_CFG_PORTS (
    id INT NOT NULL AUTO_INCREMENT,
    uniq_id INT NOT NULL COMMENT '对应MACHINE_APP_LIST中的ID',
    private_port INT NOT NULL,
    public_port INT NOT NULL,
    type VARCHAR(256) NOT NULL,
    PRIMARY KEY (id),
    KEY (uniq_id)
);

CREATE TABLE PUBLISH_APP_CFG_VOLUMES (
    id INT NOT NULL AUTO_INCREMENT,
    uniq_id INT NOT NULL COMMENT '对应MACHINE_APP_LIST中的ID',
    docker_volume VARCHAR(256) NOT NULL,
    host_volume VARCHAR(256) NOT NULL,
    PRIMARY KEY (id),
    KEY (uniq_id)
);

CREATE TABLE PUBLISH_APP_CFG_DNS (
    id INT NOT NULL AUTO_INCREMENT,
    uniq_id INT NOT NULL COMMENT '对应MACHINE_APP_LIST中的ID',
    dns VARCHAR(256) NOT NULL,
    address VARCHAR(256) NOT NULL,
    PRIMARY KEY (id),
    KEY (uniq_id)
);

CREATE TABLE PUBLISH_APP_CFG_EXTRA_CMD (
    id INT NOT NULL AUTO_INCREMENT,
    uniq_id INT NOT NULL COMMENT '对应MACHINE_APP_LIST中的ID',
    extra_cmd VARCHAR(1024) NOT NULL,
    PRIMARY KEY (id),
    KEY (uniq_id)
);