#include <xwork_dbpool_server.h>

#include <ss_service.h>

xwork_dbpool_server::xwork_dbpool_server()
	: xwork_server()
{
}

xwork_dbpool_server::xwork_dbpool_server(const char * name, con_pool * pdbpool, int nmaxexception, int works)
	: xwork_server(name, nmaxexception, works)
	, m_pdbpool(pdbpool)
	, m_pdb(0)
{
}

bool xwork_dbpool_server::close(int secs)
{
	static const char szFunctionName[] = "xwork_dbpool_server::close";

	xwork_server::close(secs);

	try{
		m_pdbpool->close(m_pdb);
		m_pdb = 0;
	}catch (...) {
		WriteToEventLog("%s : %s关闭数据库出现异常.", szFunctionName, m_name);
	}

	return true;
}

void xwork_dbpool_server::run(void)
{
	static const char szFunctionName[] = "xwork_dbpool_server::run";

	bool isok = true;
	try{
		m_pdb = m_pdbpool->open();
		if (m_pdb)
			run_db();
	}catch (oralib::error &e) {
		isok = false;
		WriteToEventLog("%s : %s", szFunctionName, (e.details()).c_str());
	}catch (...) {
		isok = false;
		WriteToEventLog("%s : %s运行出现未知异常.", szFunctionName, m_name);
	}

	try{
		m_pdbpool->close(m_pdb, isok);
	}catch (...) {
		WriteToEventLog("%s : %s关闭数据库出现异常.", szFunctionName, m_name);
	}
	m_pdb = 0;
}
