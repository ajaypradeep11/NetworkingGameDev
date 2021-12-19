#pragma once
#include <mysql/jdbc.h>
#include <string>

using namespace sql;

enum CreateAccountWebResult
{
	SUCCESS,
	ACCOUNT_ALREADY_EXIST,
	INVALID_PASSWORD,
	INTERNAL_SERVER_ERROR,
	EMAIL_NOT_FOUND,
	PASSWORD_CONDITION_CHECK
};

class DBHelper
{
public:
	DBHelper(void);

	void Connect(const std::string& hostname, const std::string& username, const std::string& password);
	bool isConnected(void);


	//select query


	//update query


	//insert
	CreateAccountWebResult loginAccount(const std::string& email, const std::string& password);

	CreateAccountWebResult CreateAccount(const std::string& email, const std::string& password, const std::string& salt, const std::string& hashedpassword);

private:
	void GeneratePreparedStatements(void);
	mysql::MySQL_Driver* m_Driver;
	Connection* m_Connection;
	ResultSet* m_ResultSet;
	bool m_isConnected;
	bool m_isConnecting;

};