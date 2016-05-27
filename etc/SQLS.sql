-- 初始化
CREATE DATABASE IF NOT EXISTS ogp DEFAULT CHARACTER SET utf8;
SET NAMES 'utf8';
USE ogp;

CREATE TABLE MACHINE_APP_LIST (
    id INT NOT NULL AUTO_INCREMENT,
    ip_address VARCHAR(32) COMMENT '主机的IP地址',
    port INT NOT NULL COMMENT '主机的端口号',
    app_id INT NOT NULL COMMENT '主机上运行的APP的ID',
    version_id INT NOT NULL COMMENT '主机上运行的APP的版本号ID',
    PRIMARY KEY(id),
    KEY(ip_address, port),
    KEY(app_id)
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
    app_id INT NOT NULL COMMENT '主机上运行的APP的id',
    version VARCHAR(1024) NOT NULL COMMENT 'APP版本',
    registe_time DATETIME NOT NULL COMMENT '版本注册时间',
    description VARCHAR(2048) NOT NULL COMMENT '版本说明',
    PRIMARY KEY(id),
    KEY (app_id)
);