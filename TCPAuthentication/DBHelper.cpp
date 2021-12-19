#include "DBHelper.h"
#include "HashGen.h"
#include <mysql/jdbc.h>
#include <ctime>

using namespace sql;

sql::PreparedStatement* GetEmails;
sql::PreparedStatement* InsertWebAuth;
sql::PreparedStatement* updateUserDateTime;
sql::PreparedStatement* updateUserLogin;
sql::PreparedStatement* GetId;

DBHelper::DBHelper(void)
	: m_isConnected(false)
	, m_isConnecting(false)
	, m_Connection(0)
	, m_Driver(0)
	, m_ResultSet(0)
{

}

bool DBHelper::isConnected(void)
{
	return m_isConnected;
}

std::string decryptPassword(std::string saltString, std::string password)
{
	std::string pass = password + saltString;
	SHA256 sha;
	sha.update(pass);
	uint8_t* digest = sha.digest();
	return SHA256::toString(digest);
}

CreateAccountWebResult DBHelper::CreateAccount(const std::string& email, const std::string& password, const std::string& salt, const std::string& hashedpassword)
{

	//sql::Statement* stmt = m_Connection->createStatement();

	GetEmails->setString(1, email);
	try
	{
		//m_ResultSet = pStmt->executeQuery("SELECT * from `web_auth`;");
		m_ResultSet = GetEmails->executeQuery();
	}
	catch (SQLException e)
	{
		printf("failllllllll\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	if (m_ResultSet->rowsCount() > 0)
	{
		printf("Account already Exist\n");
		return CreateAccountWebResult::ACCOUNT_ALREADY_EXIST;
	}

	//sql::Statement* stmt = m_Connection->createStatement();
	try
	{

		time_t now = time(NULL);
		//char *str = ctime(&now);
		char str[26] = {};
		ctime_s(str, 26, &now);

		updateUserDateTime->setDateTime(1, str);
		int result = updateUserDateTime->executeUpdate();
		int id = 1;
		m_ResultSet = GetId->executeQuery();
		if (m_ResultSet->rowsCount() > 0)
		{
			while (m_ResultSet->next()) 
			{
				id = m_ResultSet->getInt(1);
				break;
			}
		}


		std::string getSaltPassword;
		InsertWebAuth->setString(1, email);
		InsertWebAuth->setString(2, hashedpassword);
		InsertWebAuth->setString(3, salt);
		InsertWebAuth->setInt(4, id);
		int result1 = InsertWebAuth->executeUpdate();

		//m_ResultSet = GetEmails->executeQuery();
		



		/*if (result == 0) {
			printf("Failed to insert row \n");
		}
		else
		{
			printf("Successfully inserted row \n");
		}*/
	}
	catch (SQLException e)
	{
		printf("Failed to insert account into web_auth\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	//sql::Statement* stmt = m_Connection->createStatement();
	/*try
	{
		m_ResultSet = stmt->executeQuery("SELECT LAST_INSERT_ID();");
	}
	catch (SQLException e)
	{
		printf("Failed to retrive last insert id\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	int lastId = 0;
	if (m_ResultSet->next())
	{
		lastId = m_ResultSet->getInt(1);
		printf("last id: %d\n", lastId);
	}*/

	//while (m_ResultSet->next()) {
	//	SQLString id = m_ResultSet->getString(sql::SQLString("email"));

	//	printf("id: %s\n", id.c_str());

	//	/*int32_t id_keyColumn = m_ResultSet->getInt(0);

	//	printf("id keycolumn(0): %d\n", id_keyColumn);*/

	//	int32_t id_keyColumn = m_ResultSet->getInt(1);

	//	printf("id keycolumn(1): %d\n", id_keyColumn);

	//}


	printf("Sucessssssssssss\n");
	return CreateAccountWebResult::SUCCESS;

}

CreateAccountWebResult DBHelper::loginAccount(const std::string& email, const std::string& password)
{
	GetEmails->setString(1, email);
	try
	{
		//m_ResultSet = pStmt->executeQuery("SELECT * from `web_auth`;");
		m_ResultSet = GetEmails->executeQuery();
	}
	catch (SQLException e)
	{
		printf("failllllllll\n");
		return CreateAccountWebResult::INTERNAL_SERVER_ERROR;
	}

	if (m_ResultSet->rowsCount() > 0 )
	{
		while (m_ResultSet->next()) {
			/*SQLString salt = m_ResultSet->getString(sql::SQLString("salt"));
			SQLString hashedpassword = m_ResultSet->getString(sql::SQLString("hashed_password"));
			printf("salt: %s\n", salt.c_str());
			printf("hashedpassword: %s\n", hashedpassword.c_str());*/
			/*int32_t id_keyColumn = m_ResultSet->getInt(0);
			printf("id keycolumn(0): %d\n", id_keyColumn);*/

			int id = m_ResultSet->getInt(1);
			SQLString email = m_ResultSet->getString(sql::SQLString("email"));
			SQLString salt = m_ResultSet->getString(sql::SQLString("salt"));
			SQLString hashedpassword = m_ResultSet->getString(sql::SQLString("hashed_password"));
			int userid = m_ResultSet->getInt(5);
			std::string createdHashedPassword = decryptPassword(salt, password);
			if (hashedpassword == createdHashedPassword) {

				updateUserLogin->setInt(1, userid);
				updateUserLogin->executeUpdate();
				return CreateAccountWebResult::SUCCESS;
			}
			else {
				printf("Account already Exist\n");
				return CreateAccountWebResult::INVALID_PASSWORD;
			}

		}
	}
	else {
		printf("Account already Exist\n");
		return CreateAccountWebResult::EMAIL_NOT_FOUND;
	}
}

void DBHelper::GeneratePreparedStatements(void)
{
	GetEmails = m_Connection->prepareStatement("SELECT * FROM `web_auth` WHERE email = ?;");
	GetId = m_Connection->prepareStatement("select id from `user` order by id DESC");
	InsertWebAuth = m_Connection->prepareStatement("INSERT INTO web_auth(email, hashed_password, salt, userId) VALUES (?, ?, ?, ?)");
	updateUserLogin = m_Connection->prepareStatement("UPDATE `user` SET last_login = now() WHERE id = ?;");
	updateUserDateTime = m_Connection->prepareStatement("INSERT INTO user(creation_date) VALUES (?)");
}

void DBHelper::Connect(const std::string& hostname, const std::string& username, const std::string& password) {

	if (m_isConnecting)
	{
		return;
	}

	m_isConnecting = true;

	try
	{
		m_Driver = sql::mysql::get_driver_instance();
		m_Connection = m_Driver->connect(hostname, username, password);
		m_Connection->setSchema("authenticationservice");
		//sql::mysql::MySQL_Connection();
	}
	catch (sql::SQLException e)
	{
		printf("failed: %s\n", e.what());
		m_isConnecting = false;
		return;
	}

	m_isConnected = true;
	m_isConnecting = false;

	GeneratePreparedStatements();

	printf("successful\n");
}


