-- 删除旧的表（如果存在）
DROP TABLE IF EXISTS `user_profile`;
DROP VIEW IF EXISTS `basic_inf`;
DROP VIEW IF EXISTS `details_inf`;

-- 创建 user_profile 表（修复默认值）
CREATE TABLE `user_profile` (
  `ssn` char(18) NOT NULL COMMENT '社会保障号码',
  `name` char(8) NOT NULL COMMENT '患者姓名',
  `sex` char(2) NOT NULL DEFAULT '男' COMMENT '性别',
  `ethnic` char(10) NOT NULL DEFAULT '汉' COMMENT '民族',
  `birth` date NOT NULL COMMENT '出生日期',
  `address` varchar(50) DEFAULT NULL COMMENT '住址',
  `casehistory` varchar(500) DEFAULT NULL COMMENT '病例',
  `picture` blob NULL COMMENT '照片',
  PRIMARY KEY (`ssn`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;

-- 插入测试数据
INSERT INTO `user_profile` (`ssn`, `name`, `sex`, `ethnic`, `birth`, `address`, `casehistory`, `picture`) VALUES 
('123456789012345678', '张三', '男', '汉', '2000-01-01', '北京市', '感冒', NULL),
('320199101011806212', '赵国庆', '男', '汉', '1976-12-05', '南京市溪水区永阳镇刘秀路168号', NULL, NULL),
('987654321098765432', '李四', '女', '回', '1995-03-15', '上海市', '头痛', NULL);

-- 创建 basic_inf 视图
CREATE VIEW `basic_inf` AS 
SELECT 
  `ssn` AS `社会保障号码`,
  `name` AS `患者`,
  `sex` AS `性别`,
  `ethnic` AS `民族`,
  `birth` AS `出生日期`,
  `address` AS `住址`
FROM `user_profile`;

-- 创建 details_inf 视图
CREATE VIEW `details_inf` AS 
SELECT 
  `name` AS `姓名`,
  `casehistory` AS `病例`,
  `picture` AS `照片`
FROM `user_profile`;