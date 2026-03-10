-- --------------------------------------------------------
-- 호스트:                          127.0.0.1
-- 서버 버전:                        11.8.3-MariaDB-0+deb13u1 from Debian - -- Please help get to 10k stars at https://github.com/MariaDB/Server
-- 서버 OS:                        debian-linux-gnu
-- HeidiSQL 버전:                  12.15.0.7171
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;


-- Smart_MES_Core 데이터베이스 구조 내보내기
CREATE DATABASE IF NOT EXISTS `Smart_MES_Core` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_uca1400_ai_ci */;
USE `Smart_MES_Core`;

-- 테이블 Smart_MES_Core.cust_company 구조 내보내기
CREATE TABLE IF NOT EXISTS `cust_company` (
  `id` uuid NOT NULL,
  `company_name` varchar(50) NOT NULL,
  `company_address` varchar(50) DEFAULT NULL,
  `company_number` varchar(20) DEFAULT NULL,
  `purchasing_product` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.cust_company:~7 rows (대략적) 내보내기
INSERT INTO `cust_company` (`id`, `company_name`, `company_address`, `company_number`, `purchasing_product`) VALUES
	('c2adcfb6-5fae-4c77-bd34-0a7ce9cb1a72', '에어가드인더스트리', '경북 구미시 1공단로 233', '054-462-7721', 'Gas Detection Sensor Node'),
	('89a77621-3514-49fb-98e6-112d84870e3c', '모션케어시스템즈', '경남 창원시 성산구 공단로 315', '055-286-6201', 'Vibration Monitoring Sensor Node'),
	('91ada2fb-1922-11f1-9143-2ccf6710ca77', '그린팩토리솔루션', '경기 평택시 산단로 88', '031-657-2101', 'Temperature & Humidity Sensor Node'),
	('91ada623-1922-11f1-9143-2ccf6710ca77', '클린룸시스템코리아', '충북 청주시 흥덕구 직지대로 412', '043-276-4410', 'Temperature & Humidity Sensor Node'),
	('91ada6b2-1922-11f1-9143-2ccf6710ca77', '세이프플랜트엔지니어링', '울산 남구 테크노산업로 57', '052-289-5502', 'Temperature & Humidity Sensor Node'),
	('25111806-6eeb-4dbe-95ab-34c4cd412596', '프리딕티브메인터넌스랩스', '경기 안양시 동안구 시민대로 401', '031-421-9088', 'Vibration Monitoring Sensor Node'),
	('bd3b4ef0-8998-4dd4-823d-5d72d16b2c68', '코리아케미컬세이프티텍', '전남 여수시 여수산단로 101', '061-691-1184', 'Gas Detection Sensor Node');

-- 테이블 Smart_MES_Core.environment_logs 구조 내보내기
CREATE TABLE IF NOT EXISTS `environment_logs` (
  `id` uuid NOT NULL,
  `process_id` uuid DEFAULT NULL,
  `error_type` varchar(10) DEFAULT NULL,
  `sensor_type` varchar(10) DEFAULT NULL,
  `sensor_value` varchar(100) DEFAULT NULL,
  `description` text DEFAULT NULL,
  `event_at` timestamp NULL DEFAULT current_timestamp(),
  PRIMARY KEY (`id`),
  KEY `process_id` (`process_id`),
  CONSTRAINT `environment_logs_ibfk_1` FOREIGN KEY (`process_id`) REFERENCES `process` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.environment_logs:~5 rows (대략적) 내보내기
INSERT INTO `environment_logs` (`id`, `process_id`, `error_type`, `sensor_type`, `sensor_value`, `description`, `event_at`) VALUES
	('333862ad-1bb3-11f1-9143-2ccf6710ca77', '1faeda99-1186-4920-9377-4800ee156390', NULL, 'TEMP', '29.7', 'FIRE DETECTED: LOG temp=29.7', '2026-03-09 12:26:29'),
	('e6d38327-1bb4-11f1-9143-2ccf6710ca77', '1faeda99-1186-4920-9377-4800ee156390', NULL, 'TEMP', '29.5', 'FIRE DETECTED: LOG temp=29.5', '2026-03-09 12:38:40'),
	('f5fb6d26-1bb4-11f1-9143-2ccf6710ca77', '1faeda99-1186-4920-9377-4800ee156390', NULL, 'TEMP', '29.7', 'FIRE DETECTED: LOG temp=29.7', '2026-03-09 12:39:06'),
	('fae1be75-1bb4-11f1-9143-2ccf6710ca77', '63014862-9222-4f98-b74f-f6e4f5bc979a', NULL, 'TEMP', '29.7', 'FIRE DETECTED: MFG temp=29.7', '2026-03-09 12:39:14'),
	('55feaed7-1bb7-11f1-9143-2ccf6710ca77', '1faeda99-1186-4920-9377-4800ee156390', NULL, 'TEMP', '29.5', 'FIRE DETECTED: LOG temp=29.5', '2026-03-09 12:56:06');

-- 테이블 Smart_MES_Core.inventory 구조 내보내기
CREATE TABLE IF NOT EXISTS `inventory` (
  `id` uuid NOT NULL,
  `company_id` uuid DEFAULT NULL,
  `item_code` varchar(50) DEFAULT NULL,
  `item_name` varchar(50) NOT NULL,
  `current_stock` int(11) DEFAULT NULL,
  `min_stock_level` int(11) NOT NULL,
  `max_stock_level` int(11) DEFAULT NULL,
  `unit` varchar(20) DEFAULT NULL,
  `location` varchar(10) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `item_code` (`item_code`),
  KEY `company_id` (`company_id`),
  CONSTRAINT `inventory_ibfk_1` FOREIGN KEY (`company_id`) REFERENCES `supp_company` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.inventory:~4 rows (대략적) 내보내기
INSERT INTO `inventory` (`id`, `company_id`, `item_code`, `item_name`, `current_stock`, `min_stock_level`, `max_stock_level`, `unit`, `location`) VALUES
	('7501abdd-d9b8-4ba0-80b8-27ec059db933', 'c930d2ef-1e6a-4fab-a303-ed5aa42445f2', 'S1', 'Sensor Housing', 58, 30, 120, 'KG', 'WH-A01'),
	('504f5fef-6f69-4a51-be39-33200ebf9ebe', '69e998a1-9f56-45db-8d25-3c1368cdc87f', 'S2', 'Sensor Element Module', 20, 10, 80, 'EA', 'WH-B02'),
	('80b79980-29f1-4d47-963b-4796ff3f816d', 'c930d2ef-1e6a-4fab-a303-ed5aa42445f2', 'S3', 'MCU Control Board', 32, 20, 100, 'EA', 'WH-A01'),
	('29738312-4709-4f18-9ab1-89326d4a8c3b', 'c801c548-8784-40b8-9a24-424d4ac43990', 'S4', 'Communication Module', 54, 10, 60, 'EA', 'WH-C03');

-- 테이블 Smart_MES_Core.inventory_order_logs 구조 내보내기
CREATE TABLE IF NOT EXISTS `inventory_order_logs` (
  `id` uuid NOT NULL,
  `user_id` uuid DEFAULT NULL,
  `item_id` uuid DEFAULT NULL,
  `item_code` varchar(50) DEFAULT NULL,
  `item_name` varchar(50) DEFAULT NULL,
  `stock` int(11) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT current_timestamp(),
  `receive_at` timestamp NULL DEFAULT NULL COMMENT '입고 날짜',
  `updated_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `item_id` (`item_id`),
  CONSTRAINT `inventory_order_logs_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `inventory` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.inventory_order_logs:~30 rows (대략적) 내보내기
INSERT INTO `inventory_order_logs` (`id`, `user_id`, `item_id`, `item_code`, `item_name`, `stock`, `status`, `created_at`, `receive_at`, `updated_at`) VALUES
	('790167aa-1ac1-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 20, 'DONE', '2026-03-08 07:36:08', '2026-03-16 15:00:00', '2026-03-08 07:36:48'),
	('ae1c4241-1ac1-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '29738312-4709-4f18-9ab1-89326d4a8c3b', 'S4', 'Communication Module', 32, 'DONE', '2026-03-08 07:37:37', '2026-03-09 15:00:00', '2026-03-08 07:41:29'),
	('b124b7e8-1ac2-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 10, 'DONE', '2026-03-08 07:44:52', '2026-03-11 15:00:00', '2026-03-08 07:52:48'),
	('b4d2bfbb-1ac2-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 15, 'DONE', '2026-03-08 07:44:58', '2026-03-18 15:00:00', '2026-03-08 07:47:14'),
	('c7522f85-1ac2-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 10, 'DONE', '2026-03-08 07:45:29', '2026-03-10 15:00:00', '2026-03-08 07:46:38'),
	('da3f1384-1ac3-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '504f5fef-6f69-4a51-be39-33200ebf9ebe', 'S2', 'Sensor Element Module', 10, 'DONE', '2026-03-08 07:53:10', '2026-03-24 15:00:00', '2026-03-08 07:53:40'),
	('13d52ea2-1ac4-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 18, 'DONE', '2026-03-08 07:54:47', '2026-03-10 15:00:00', '2026-03-08 07:55:18'),
	('8c023e81-1ac4-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 15, 'DONE', '2026-03-08 07:58:09', '2026-03-07 15:00:00', '2026-03-08 07:58:40'),
	('e3c5aa8b-1ac8-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 15, 'DONE', '2026-03-08 08:29:14', '2026-03-10 15:00:00', '2026-03-08 08:31:10'),
	('dc93e37f-1ad1-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 10, 'DONE', '2026-03-08 09:33:27', '2026-03-07 15:00:00', '2026-03-08 09:33:58'),
	('b68ad653-1ad8-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '504f5fef-6f69-4a51-be39-33200ebf9ebe', 'S2', 'Sensor Element Module', 20, 'DONE', '2026-03-08 10:22:30', '2026-03-10 15:00:00', '2026-03-09 02:49:41'),
	('3ea73b3d-1add-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 20, 'DONE', '2026-03-08 10:54:56', '2026-03-07 15:00:00', '2026-03-09 00:23:01'),
	('ddc9858f-1b4c-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 20, 'DONE', '2026-03-09 00:13:57', '2026-03-08 15:00:00', '2026-03-09 02:35:10'),
	('f9ad2039-1b66-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '504f5fef-6f69-4a51-be39-33200ebf9ebe', 'S2', 'Sensor Element Module', 20, 'DONE', '2026-03-09 03:20:51', '2026-03-09 15:00:00', '2026-03-09 03:21:33'),
	('2bc9a348-1b69-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 10, 'DONE', '2026-03-09 03:36:34', '2026-03-17 15:00:00', '2026-03-09 03:37:01'),
	('7b150d2f-1b69-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 11, 'DONE', '2026-03-09 03:38:47', '2026-03-18 15:00:00', '2026-03-09 03:39:31'),
	('4343ef9a-1b6a-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 10, 'DONE', '2026-03-09 03:44:23', '2026-03-17 15:00:00', '2026-03-09 03:44:50'),
	('65d5dc85-1b6a-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '29738312-4709-4f18-9ab1-89326d4a8c3b', 'S4', 'Communication Module', 5, 'DONE', '2026-03-09 03:45:21', '2026-03-08 15:00:00', '2026-03-09 03:45:40'),
	('c50cd8a2-1b6a-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '29738312-4709-4f18-9ab1-89326d4a8c3b', 'S4', 'Communication Module', 5, 'DONE', '2026-03-09 03:48:01', '2026-03-08 15:00:00', '2026-03-09 03:48:23'),
	('f2308460-1b6a-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 5, 'DONE', '2026-03-09 03:49:16', '2026-03-08 15:00:00', '2026-03-09 03:49:37'),
	('fb07070e-1b76-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 20, 'DONE', '2026-03-09 05:15:25', '2026-03-08 15:00:00', '2026-03-09 05:16:03'),
	('18ab5d7a-1b77-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '504f5fef-6f69-4a51-be39-33200ebf9ebe', 'S2', 'Sensor Element Module', 12, 'DONE', '2026-03-09 05:16:15', '2026-03-08 15:00:00', '2026-03-09 05:16:43'),
	('78ad8683-1b78-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '504f5fef-6f69-4a51-be39-33200ebf9ebe', 'S2', 'Sensor Element Module', 30, 'DONE', '2026-03-09 05:26:05', '2026-03-08 15:00:00', '2026-03-09 05:26:53'),
	('ed3551ca-1b78-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 15, 'DONE', '2026-03-09 05:29:21', '2026-03-08 15:00:00', '2026-03-09 05:29:52'),
	('a0edfad6-1baa-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 10, 'DONE', '2026-03-09 11:25:08', '2026-03-08 15:00:00', '2026-03-09 11:31:05'),
	('2353cab9-1bb6-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 7, 'DONE', '2026-03-09 12:47:31', '2026-03-09 15:00:00', '2026-03-09 12:48:14'),
	('e6a84828-1c14-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '7501abdd-d9b8-4ba0-80b8-27ec059db933', 'S1', 'Sensor Housing', 30, 'DONE', '2026-03-10 00:05:51', '2026-03-09 15:00:00', '2026-03-10 00:07:10'),
	('ea52047a-1c14-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '504f5fef-6f69-4a51-be39-33200ebf9ebe', 'S2', 'Sensor Element Module', 20, 'DONE', '2026-03-10 00:05:58', '2026-03-09 15:00:00', '2026-03-10 00:07:03'),
	('ee1179e4-1c14-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '80b79980-29f1-4d47-963b-4796ff3f816d', 'S3', 'MCU Control Board', 15, 'DONE', '2026-03-10 00:06:04', '2026-03-09 15:00:00', '2026-03-10 00:07:35'),
	('23d0c908-1c15-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', '29738312-4709-4f18-9ab1-89326d4a8c3b', 'S4', 'Communication Module', 30, 'DONE', '2026-03-10 00:07:34', '2026-03-09 15:00:00', '2026-03-10 00:08:23');

-- 테이블 Smart_MES_Core.process 구조 내보내기
CREATE TABLE IF NOT EXISTS `process` (
  `id` uuid NOT NULL,
  `process_name` varchar(20) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.process:~3 rows (대략적) 내보내기
INSERT INTO `process` (`id`, `process_name`) VALUES
	('1faeda99-1186-4920-9377-4800ee156390', 'logistics_line'),
	('9f87460c-14f2-4f46-a19f-dc53890d8ee1', 'factory'),
	('63014862-9222-4f98-b74f-f6e4f5bc979a', 'manufacture_line');

-- 테이블 Smart_MES_Core.product 구조 내보내기
CREATE TABLE IF NOT EXISTS `product` (
  `id` uuid NOT NULL,
  `product_code` varchar(50) DEFAULT NULL,
  `product_name` varchar(50) NOT NULL,
  `product_stock` int(11) DEFAULT NULL,
  `description` text DEFAULT NULL,
  `recipe` varchar(50) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `product_code` (`product_code`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.product:~3 rows (대략적) 내보내기
INSERT INTO `product` (`id`, `product_code`, `product_name`, `product_stock`, `description`, `recipe`) VALUES
	('43daf552-1922-11f1-9143-2ccf6710ca77', 'PRD-ALU-001', 'Temperature & Humidity Sensor Node', 53, '알루미늄 소재 프레임', '1, 1, 1, 1'),
	('43daf806-1922-11f1-9143-2ccf6710ca77', 'PRD-WIR-001', 'Gas Detection Sensor Node', 20, '전장 와이어 하네스', '1, 2, 1, 1'),
	('43daf8af-1922-11f1-9143-2ccf6710ca77', 'PRD-SEN-001', 'Vibrating Monitoring Sensor Node', 20, 'ABS 휠 속도 센서 모듈', '1, 2, 2, 1');

-- 테이블 Smart_MES_Core.product_deli_logs 구조 내보내기
CREATE TABLE IF NOT EXISTS `product_deli_logs` (
  `id` uuid NOT NULL,
  `company_id` uuid DEFAULT NULL,
  `product_id` uuid DEFAULT NULL,
  `delivery_stock` int(11) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT current_timestamp(),
  `updated_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `company_id` (`company_id`),
  KEY `product_id` (`product_id`),
  CONSTRAINT `product_deli_logs_ibfk_1` FOREIGN KEY (`company_id`) REFERENCES `cust_company` (`id`),
  CONSTRAINT `product_deli_logs_ibfk_2` FOREIGN KEY (`product_id`) REFERENCES `product` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.product_deli_logs:~13 rows (대략적) 내보내기
INSERT INTO `product_deli_logs` (`id`, `company_id`, `product_id`, `delivery_stock`, `status`, `created_at`, `updated_at`) VALUES
	('aa49248f-1922-11f1-9143-2ccf6710ca77', '91ada623-1922-11f1-9143-2ccf6710ca77', '43daf806-1922-11f1-9143-2ccf6710ca77', 4, 'DONE', '2026-03-06 06:06:50', '2026-03-06 06:07:42'),
	('ed7d0fd0-1922-11f1-9143-2ccf6710ca77', '91ada2fb-1922-11f1-9143-2ccf6710ca77', '43daf806-1922-11f1-9143-2ccf6710ca77', 555, 'DONE', '2026-03-06 06:08:42', '2026-03-06 06:08:45'),
	('0390055d-1923-11f1-9143-2ccf6710ca77', '91ada6b2-1922-11f1-9143-2ccf6710ca77', '43daf8af-1922-11f1-9143-2ccf6710ca77', 222, 'DONE', '2026-03-06 06:09:19', '2026-03-06 06:09:23'),
	('1704a365-1923-11f1-9143-2ccf6710ca77', '91ada2fb-1922-11f1-9143-2ccf6710ca77', '43daf552-1922-11f1-9143-2ccf6710ca77', 555, 'DONE', '2026-03-06 06:09:52', '2026-03-06 06:10:01'),
	('9fdb8556-1930-11f1-9143-2ccf6710ca77', '91ada623-1922-11f1-9143-2ccf6710ca77', '43daf8af-1922-11f1-9143-2ccf6710ca77', 33, 'DONE', '2026-03-06 07:46:45', '2026-03-06 07:46:49'),
	('5185efa6-1aa4-11f1-9143-2ccf6710ca77', '91ada2fb-1922-11f1-9143-2ccf6710ca77', '43daf552-1922-11f1-9143-2ccf6710ca77', 10, 'DONE', '2026-03-08 04:07:26', '2026-03-08 04:07:35'),
	('12dc7a02-1b7f-11f1-9143-2ccf6710ca77', '91ada623-1922-11f1-9143-2ccf6710ca77', '43daf552-1922-11f1-9143-2ccf6710ca77', 15, 'DONE', '2026-03-09 06:13:21', '2026-03-09 06:13:28'),
	('f28c716c-1b82-11f1-9143-2ccf6710ca77', '91ada623-1922-11f1-9143-2ccf6710ca77', '43daf8af-1922-11f1-9143-2ccf6710ca77', 1, 'DONE', '2026-03-09 06:41:05', '2026-03-09 06:41:07'),
	('f91d4a40-1b82-11f1-9143-2ccf6710ca77', 'c2adcfb6-5fae-4c77-bd34-0a7ce9cb1a72', '43daf552-1922-11f1-9143-2ccf6710ca77', 111, 'PENDING', '2026-03-09 06:41:16', NULL),
	('cf5af310-1b95-11f1-9143-2ccf6710ca77', '89a77621-3514-49fb-98e6-112d84870e3c', '43daf552-1922-11f1-9143-2ccf6710ca77', 100, 'PENDING', '2026-03-09 08:56:06', NULL),
	('992cf3ee-1b9a-11f1-9143-2ccf6710ca77', 'c2adcfb6-5fae-4c77-bd34-0a7ce9cb1a72', '43daf552-1922-11f1-9143-2ccf6710ca77', 55, 'PENDING', '2026-03-09 09:30:23', NULL),
	('b05a5a7a-1bab-11f1-9143-2ccf6710ca77', 'c2adcfb6-5fae-4c77-bd34-0a7ce9cb1a72', '43daf552-1922-11f1-9143-2ccf6710ca77', 4, 'DONE', '2026-03-09 11:32:43', '2026-03-09 11:33:20'),
	('52b5bb44-1bb6-11f1-9143-2ccf6710ca77', '91ada623-1922-11f1-9143-2ccf6710ca77', '43daf806-1922-11f1-9143-2ccf6710ca77', 5, 'DONE', '2026-03-09 12:48:51', '2026-03-09 12:48:55');

-- 테이블 Smart_MES_Core.product_items 구조 내보내기
CREATE TABLE IF NOT EXISTS `product_items` (
  `id` uuid NOT NULL,
  `item_id` uuid DEFAULT NULL,
  `product_id` uuid DEFAULT NULL,
  `quantity_required` int(11) NOT NULL,
  `unit` varchar(20) DEFAULT NULL,
  `description` text DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `item_id` (`item_id`),
  KEY `product_id` (`product_id`),
  CONSTRAINT `product_items_ibfk_1` FOREIGN KEY (`item_id`) REFERENCES `inventory` (`id`),
  CONSTRAINT `product_items_ibfk_2` FOREIGN KEY (`product_id`) REFERENCES `product` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.product_items:~0 rows (대략적) 내보내기

-- 테이블 Smart_MES_Core.product_logs 구조 내보내기
CREATE TABLE IF NOT EXISTS `product_logs` (
  `id` uuid NOT NULL,
  `order_id` uuid DEFAULT NULL,
  `user_id` uuid DEFAULT NULL,
  `assignment_part` varchar(50) DEFAULT NULL,
  `motor_speed` int(11) DEFAULT NULL,
  `prod_count` int(11) DEFAULT NULL,
  `defect_count` int(11) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `started_at` timestamp NULL DEFAULT NULL,
  `ended_at` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  KEY `order_id` (`order_id`),
  KEY `user_id` (`user_id`),
  CONSTRAINT `product_logs_ibfk_1` FOREIGN KEY (`order_id`) REFERENCES `product_order_logs` (`id`),
  CONSTRAINT `product_logs_ibfk_2` FOREIGN KEY (`user_id`) REFERENCES `user` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.product_logs:~17 rows (대략적) 내보내기
INSERT INTO `product_logs` (`id`, `order_id`, `user_id`, `assignment_part`, `motor_speed`, `prod_count`, `defect_count`, `status`, `started_at`, `ended_at`) VALUES
	('c306b0d7-1b4c-11f1-9143-2ccf6710ca77', 'b9ff0a01-1b4c-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 10, 12, 'DONE', '2026-03-09 00:13:12', '2026-03-09 00:15:03'),
	('260b15e0-1b50-11f1-9143-2ccf6710ca77', '1a29f630-1b50-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 9, 8, 'DONE', '2026-03-09 00:37:27', '2026-03-09 00:38:57'),
	('b0b89eea-1b62-11f1-9143-2ccf6710ca77', '62019fc2-1b60-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 20, 10, 'DONE', '2026-03-09 02:50:11', '2026-03-09 02:52:41'),
	('2c2787a9-1b67-11f1-9143-2ccf6710ca77', '228822cb-1b67-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 5, 0, 'DONE', '2026-03-09 03:22:16', '2026-03-09 03:22:41'),
	('e124420a-1b76-11f1-9143-2ccf6710ca77', 'd88d18c2-1b76-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 5, 1, 'DONE', '2026-03-09 05:14:42', '2026-03-09 05:15:12'),
	('5b88b6e7-1b77-11f1-9143-2ccf6710ca77', '554a3f4e-1b77-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 5, 4, 'DONE', '2026-03-09 05:18:07', '2026-03-09 05:18:53'),
	('8e85ee4e-1b77-11f1-9143-2ccf6710ca77', '8990f6de-1b77-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 1, 0, 'DONE', '2026-03-09 05:19:33', '2026-03-09 05:19:38'),
	('45c58725-1b78-11f1-9143-2ccf6710ca77', '3b96719c-1b78-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 3, 0, 'DONE', '2026-03-09 05:24:40', '2026-03-09 05:24:55'),
	('9e1e5812-1b78-11f1-9143-2ccf6710ca77', '98380bdd-1b78-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 5, 1, 'DONE', '2026-03-09 05:27:08', '2026-03-09 05:27:39'),
	('98f7686f-1b7a-11f1-9143-2ccf6710ca77', '8f6346cb-1b7a-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 3, 6, 'DONE', '2026-03-09 05:41:19', '2026-03-09 05:42:04'),
	('fb8a1957-1bb6-11f1-9143-2ccf6710ca77', 'f2d881f9-1bb6-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 3, 2, 'DONE', '2026-03-09 12:53:34', '2026-03-09 12:53:59'),
	('0a8b68d0-1bb7-11f1-9143-2ccf6710ca77', 'eaabf1d5-1bb6-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 2, 1, 'DONE', '2026-03-09 12:53:59', '2026-03-09 12:54:14'),
	('781fffad-1bb7-11f1-9143-2ccf6710ca77', '76e496d6-1bb7-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 2, 2, 'INPROC', '2026-03-09 12:57:03', NULL),
	('a05b574d-1bb8-11f1-9143-2ccf6710ca77', '9f200239-1bb8-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 3, 1, 'DONE', '2026-03-09 13:05:20', '2026-03-09 13:05:40'),
	('ae926963-1c14-11f1-9143-2ccf6710ca77', 'ae12d142-1c14-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 2, 0, 'DONE', '2026-03-10 00:04:17', '2026-03-10 00:04:28'),
	('b4e94643-1c14-11f1-9143-2ccf6710ca77', 'a91c2e80-1c14-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 3, 2, 'DONE', '2026-03-10 00:04:28', '2026-03-10 00:04:53'),
	('ca42313d-1c14-11f1-9143-2ccf6710ca77', 'c9b98ae7-1c14-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 4, 3, 'DONE', '2026-03-10 00:05:04', '2026-03-10 00:05:40'),
	('03b88132-1c16-11f1-9143-2ccf6710ca77', '01fc5f17-1c16-11f1-9143-2ccf6710ca77', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'MFG', 100, 3, 3, 'DONE', '2026-03-10 00:13:50', '2026-03-10 00:14:20');

-- 테이블 Smart_MES_Core.product_order_logs 구조 내보내기
CREATE TABLE IF NOT EXISTS `product_order_logs` (
  `id` uuid NOT NULL,
  `user_id` uuid DEFAULT NULL,
  `product_id` uuid DEFAULT NULL,
  `order_count` int(11) DEFAULT NULL,
  `motor_speed` int(11) DEFAULT NULL,
  `status` varchar(10) DEFAULT NULL,
  `created_at` timestamp NULL DEFAULT current_timestamp(),
  `updated_at` timestamp NULL DEFAULT NULL,
  `deadline_at` timestamp NULL DEFAULT NULL COMMENT '주문 완료 목표 시각(데드라인)',
  PRIMARY KEY (`id`),
  KEY `user_id` (`user_id`),
  KEY `product_id` (`product_id`),
  CONSTRAINT `product_order_logs_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`id`),
  CONSTRAINT `product_order_logs_ibfk_2` FOREIGN KEY (`product_id`) REFERENCES `product` (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.product_order_logs:~17 rows (대략적) 내보내기
INSERT INTO `product_order_logs` (`id`, `user_id`, `product_id`, `order_count`, `motor_speed`, `status`, `created_at`, `updated_at`, `deadline_at`) VALUES
	('b9ff0a01-1b4c-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 10, 100, 'DONE', '2026-03-09 00:12:57', '2026-03-09 00:15:03', '2026-03-12 00:12:51'),
	('1a29f630-1b50-11f1-9143-2ccf6710ca77', NULL, '43daf806-1922-11f1-9143-2ccf6710ca77', 10, 100, 'DONE', '2026-03-09 00:37:07', '2026-03-09 00:38:57', '2026-03-15 00:36:54'),
	('62019fc2-1b60-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 20, 100, 'DONE', '2026-03-09 02:33:40', '2026-03-09 02:52:41', '2026-03-13 02:33:32'),
	('228822cb-1b67-11f1-9143-2ccf6710ca77', NULL, '43daf806-1922-11f1-9143-2ccf6710ca77', 5, 100, 'DONE', '2026-03-09 03:22:00', '2026-03-09 03:22:41', '2026-03-09 03:21:50'),
	('d88d18c2-1b76-11f1-9143-2ccf6710ca77', NULL, '43daf8af-1922-11f1-9143-2ccf6710ca77', 5, 100, 'DONE', '2026-03-09 05:14:27', '2026-03-09 05:15:12', '2026-03-14 05:14:18'),
	('554a3f4e-1b77-11f1-9143-2ccf6710ca77', NULL, '43daf806-1922-11f1-9143-2ccf6710ca77', 5, 100, 'DONE', '2026-03-09 05:17:57', '2026-03-09 05:18:53', '2026-03-09 05:17:50'),
	('8990f6de-1b77-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 1, 100, 'DONE', '2026-03-09 05:19:24', '2026-03-09 05:19:38', '2026-03-09 05:19:19'),
	('3b96719c-1b78-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 3, 100, 'DONE', '2026-03-09 05:24:23', '2026-03-09 05:24:55', '2026-03-27 05:25:07'),
	('98380bdd-1b78-11f1-9143-2ccf6710ca77', NULL, '43daf8af-1922-11f1-9143-2ccf6710ca77', 5, 100, 'DONE', '2026-03-09 05:26:58', '2026-03-09 05:27:39', '2026-03-09 05:26:52'),
	('8f6346cb-1b7a-11f1-9143-2ccf6710ca77', NULL, '43daf8af-1922-11f1-9143-2ccf6710ca77', 3, 100, 'DONE', '2026-03-09 05:41:03', '2026-03-09 05:42:04', '2026-03-09 05:40:52'),
	('eaabf1d5-1bb6-11f1-9143-2ccf6710ca77', NULL, '43daf806-1922-11f1-9143-2ccf6710ca77', 2, 100, 'DONE', '2026-03-09 12:53:06', '2026-03-09 12:54:14', '2026-03-12 12:54:49'),
	('f2d881f9-1bb6-11f1-9143-2ccf6710ca77', NULL, '43daf8af-1922-11f1-9143-2ccf6710ca77', 3, 100, 'DONE', '2026-03-09 12:53:19', '2026-03-09 12:53:59', '2026-03-11 12:53:07'),
	('76e496d6-1bb7-11f1-9143-2ccf6710ca77', NULL, '43daf806-1922-11f1-9143-2ccf6710ca77', 3, 100, 'INPROC', '2026-03-09 12:57:01', '2026-03-09 12:57:03', '2026-03-12 12:56:53'),
	('9f200239-1bb8-11f1-9143-2ccf6710ca77', NULL, '43daf806-1922-11f1-9143-2ccf6710ca77', 3, 100, 'DONE', '2026-03-09 13:05:18', '2026-03-09 13:05:40', '2026-03-09 13:05:12'),
	('a91c2e80-1c14-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 3, 100, 'DONE', '2026-03-10 00:04:08', '2026-03-10 00:04:53', '2026-03-20 00:04:02'),
	('ae12d142-1c14-11f1-9143-2ccf6710ca77', NULL, '43daf8af-1922-11f1-9143-2ccf6710ca77', 2, 100, 'DONE', '2026-03-10 00:04:17', '2026-03-10 00:04:28', '2026-03-22 00:04:09'),
	('c9b98ae7-1c14-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 4, 100, 'DONE', '2026-03-10 00:05:03', '2026-03-10 00:05:40', '2026-03-10 00:04:57'),
	('01fc5f17-1c16-11f1-9143-2ccf6710ca77', NULL, '43daf552-1922-11f1-9143-2ccf6710ca77', 3, 100, 'DONE', '2026-03-10 00:13:47', '2026-03-10 00:14:20', '2026-03-12 00:13:39');

-- 테이블 Smart_MES_Core.supp_company 구조 내보내기
CREATE TABLE IF NOT EXISTS `supp_company` (
  `id` uuid NOT NULL,
  `company_name` varchar(50) NOT NULL,
  `company_address` varchar(50) DEFAULT NULL,
  `company_number` varchar(20) DEFAULT NULL,
  `Item` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.supp_company:~4 rows (대략적) 내보내기
INSERT INTO `supp_company` (`id`, `company_name`, `company_address`, `company_number`, `Item`) VALUES
	('2947a320-5a25-4110-bf32-21f4a75e0f4b', '유니링크커뮤니케이션', '서울 금천구 가산디지털1로 168', '02-861-7732', 'Communication Module'),
	('69e998a1-9f56-45db-8d25-3c1368cdc87f', '한빛정밀케이스', '경상북도 경주시 외동읍 산업로 1542', '054-777-9100', 'Sensor Housing'),
	('c801c548-8784-40b8-9a24-424d4ac43990', '미래센서테크', '대구광역시 달성군 구지면 국가산단대로 52길 14', '053-610-4500', 'Sensing Module'),
	('c930d2ef-1e6a-4fab-a303-ed5aa42445f2', '네오임베디드시스템즈', '경기도 화성시 팔탄면 서해로 112-7', '031-356-7821', 'MCU Board');

-- 테이블 Smart_MES_Core.user 구조 내보내기
CREATE TABLE IF NOT EXISTS `user` (
  `id` uuid NOT NULL,
  `user_name` varchar(20) NOT NULL,
  `role` varchar(10) DEFAULT NULL,
  `process_id` uuid DEFAULT NULL,
  `face_featured_path` varchar(50) DEFAULT NULL,
  `rfid` varchar(10) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.user:~3 rows (대략적) 내보내기
INSERT INTO `user` (`id`, `user_name`, `role`, `process_id`, `face_featured_path`, `rfid`) VALUES
	('1e7f428d-cc97-4bf8-9db0-4cdfd6bb2971', 'test_user', 'SYS_ADMIN', NULL, NULL, NULL),
	('45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'test', 'SYS_ADMIN', NULL, NULL, NULL);

-- 테이블 Smart_MES_Core.user_password 구조 내보내기
CREATE TABLE IF NOT EXISTS `user_password` (
  `id` uuid NOT NULL,
  `user_id` uuid NOT NULL,
  `password_hash` varchar(128) NOT NULL,
  `salt` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `user_id` (`user_id`),
  CONSTRAINT `user_password_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`id`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_uca1400_ai_ci;

-- 테이블 데이터 Smart_MES_Core.user_password:~3 rows (대략적) 내보내기
INSERT INTO `user_password` (`id`, `user_id`, `password_hash`, `salt`) VALUES
	('65675345-815e-4f85-b296-158fa0582935', '45b5e2f9-77a5-4954-8edb-d1d4a20ce965', 'b8e3c07a4bd97a87f46fe01301bcabccdf1ab5876a007dd3260779747cd4f71d837ceb5929e369fe667af755500792f0c96b26ee6b5b90a73fcff98076a5e2ea', 'e52676d1070a4703'),
	('a5929379-90f3-4dd6-a532-4ddfde6c5e17', '1e7f428d-cc97-4bf8-9db0-4cdfd6bb2971', 'fea2f89c15b05857d514e1cadb0f07feb0b94e5b27a9cff9f11bfe62304cdff00688a8a8111f26a9a676aa558af79ebb2bca9531067c147ddb59a0ec61a0f21e', '27b9081bf891cf14');

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
