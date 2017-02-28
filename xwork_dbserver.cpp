#include <xsys_oci.h>
#include <ss_service.h>

#include "xwork_dbserver.h"

xwork_dbserver::xwork_dbserver()
	: xwork_server()
{
}

xwork_dbserver::xwork_dbserver(const char * name, const char * dbstring, int nmaxexception, int works)
	: xwork_server(name, nmaxexception, works)
	, m_pdbstring(dbstring)
{
}

bool xwork_dbserver::close(int secs)
{
	static const char szFunctionName[] = "xwork_dbserver::close";

	xwork_server::close(secs);

	try{
		m_db.close();
	}catch (...) {
		WriteToEventLog("%s : %s关闭数据库出现异常.", szFunctionName, m_name);
	}

	return true;
}

void xwork_dbserver::run(void)
{
	static const char szFunctionName[] = "xwork_dbserver::run";

	bool isok = true;
	try{
		m_db.open(m_pdbstring);
		run_db();
	}catch (oralib::error &e) {
		isok = false;
		WriteToEventLog("%s : %s", szFunctionName, (e.details()).c_str());
	}catch (...) {
		isok = false;
		WriteToEventLog("%s : %s运行出现未知异常.", szFunctionName, m_name);
	}

	try{
		if (!isok) {
			m_db.rollback();
		}
		m_db.close();
	}catch (...) {
		WriteToEventLog("%s : %s关闭数据库出现异常.", szFunctionName, m_name);
	}
}

