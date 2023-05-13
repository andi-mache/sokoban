// Copyright 2023 ShenMian
// License(Apache-2.0)

#pragma once

#include <SQLiteCpp/Database.h>
#include <filesystem>
#include <optional>

class Database
{
public:
	Database(const std::filesystem::path& path) : database_(path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
	{
		setup();
	}

	/**
	 * @brief 配置数据库.
	 */
	void setup()
	{
		database_.exec("CREATE TABLE IF NOT EXISTS tb_level ("
		               "	id INTEGER PRIMARY KEY AUTOINCREMENT,"
		               "	title  TEXT,"
		               "	author TEXT,"
		               "	map    TEXT NOT NULL,"
		               "	crc32  INTEGER NOT NULL,"
		               "	answer TEXT,"
		               "	date   DATE NOT NULL"
		               ")");
		database_.exec("CREATE TABLE IF NOT EXISTS tb_history ("
		               "	level_id  INTEGER UNIQUE,"
		               "	movements TEXT,"
		               "	datetime  DATETIME NOT NULL,"
		               "	FOREIGN KEY(level_id) REFERENCES tb_level(id)"
		               ")");
	}

	/**
	 * @brief 重置数据库.
	 */
	void reset()
	{
		database_.exec("DROP TABLE IF EXISTS tb_level, tb_history");
		setup();
	}

	/**
	 * @brief 导入关卡.
	 *
	 * @param level 关卡.
	 */
	void import_level(const Level& level)
	{
		SQLite::Statement query_level(database_, "SELECT * FROM tb_level "
		                                         "WHERE crc32 = ?");
		SQLite::Statement add_level(database_, "INSERT INTO tb_level(title, author, map, crc32, date) "
		                                       "VALUES (?, ?, ?, ?, DATE('now'))");
		query_level.bind(1, level.crc32());
		if(query_level.executeStep())
			return;

		if(level.metadata().contains("title"))
			add_level.bind(1, level.metadata().at("title"));
		if(level.metadata().contains("author"))
			add_level.bind(2, level.metadata().at("author"));
		add_level.bind(3, level.map());
		add_level.bind(4, level.crc32());
		add_level.exec();
	}

	/**
	 * @brief 从文件导入关卡.
	 *
	 * @param path XSB 格式文件路径.
	 */
	void import_levels_from_file(const std::filesystem::path& path)
	{
		const auto levels = Level::load(path);
		for(const auto& level : levels)
			import_level(level);
	}

	/**
	 * @brief 获取关卡 ID.
	 *
	 * @param level 关卡.
	 */
	std::optional<int> get_level_id(const Level& level)
	{
		SQLite::Statement query_id(database_, "SELECT id FROM tb_level "
		                                      "WHERE crc32 = ?");
		query_id.bind(1, level.crc32());
		if(!query_id.executeStep())
			return std::nullopt;
		return query_id.getColumn("id");
	}

	/**
	 * @brief 通过 ID 获取关卡.
	 *
	 * @param level 关卡.
	 */
	std::optional<Level> get_level_by_id(int id)
	{
		SQLite::Statement query_level(database_, "SELECT title, author, map, answer FROM tb_level "
		                                         "WHERE id = ?");
		query_level.bind(1, id);
		if(!query_level.executeStep())
			return std::nullopt;
		std::string data;
		if(!query_level.getColumn("title").isNull())
			data += "Title: " + query_level.getColumn("title") + '\n';
		if(!query_level.getColumn("author").isNull())
			data += "Author: " + query_level.getColumn("author") + '\n';
		if(!query_level.getColumn("answer").isNull())
			data += "Answer: " + query_level.getColumn("answer") + '\n';
		data += query_level.getColumn("map").getString();
		return Level(data);
	}

	bool update_level_answer(Level level)
	{
		SQLite::Statement update_answer(database_, "UPDATE tb_level "
		                                           "SET answer = ? "
		                                           "WHERE id = ?");
		update_answer.bind(1, level.movements());
		level.reset();
		update_answer.bind(2, get_level_id(level).value());
		return update_answer.exec();
	}

	bool update_history_movements(Level level)
	{
		SQLite::Statement update_movements(database_, "UPDATE tb_history "
		                                              "SET movements = ? "
		                                              "WHERE level_id = ?");
		update_movements.bind(1, level.movements());
		level.reset();
		update_movements.bind(2, get_level_id(level).value());
		return update_movements.exec();
	}

	void add_level_history(const Level& level)
	{
		SQLite::Statement add_history(database_, "INSERT INTO tb_history(level_id, datetime) "
		                                         "SELECT ?, DATETIME('now') "
		                                         "WHERE NOT EXISTS("
		                                         "	SELECT 1 FROM tb_history WHERE level_id = ?"
		                                         ")");
		add_history.bind(1, get_level_id(level).value());
		add_history.bind(2, get_level_id(level).value());
		add_history.exec();
	}

	/**
	 * @brief 获取历史最新关卡 ID.
	 */
	std::optional<int> get_latest_level_id()
	{
		SQLite::Statement query_latest_history(database_, "SELECT level_id FROM tb_history "
		                                                  "ORDER BY datetime DESC "
		                                                  "LIMIT 1");
		if(!query_latest_history.executeStep())
			return std::nullopt;
		return query_latest_history.getColumn("level_id");
	}

	std::string get_level_history_movements(const Level& level)
	{
		SQLite::Statement query_movements(database_, "SELECT movements FROM tb_history "
		                                             "WHERE level_id = ?");
		query_movements.bind(1, get_level_id(level).value());
		if(!query_movements.executeStep())
			return "";
		return query_movements.getColumn(0);
	}

private:
	SQLite::Database database_;
};