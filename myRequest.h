//---------------------------------------------------------------------------
#include <my_json/Request.h>
#include <System.Classes.hpp>
#include <map>
#include <vector>
#include <codecvt>
#ifndef myRequestH
#define myRequestH
class myRequest : public Request
{

	static TComboBox   *cmbField;
	static TComboBox   *cmbText;
	static TCheckBox   *chkSql;
	static TCheckBox   *chkExc;
	static TRadioGroup *rgTypeT;

	static std::map<std::string,std::map<std::string, Request>> mTemplates;



   public:

	myRequest(String _field, String _text, int _type, bool _sql, bool _exc);

	static void init(TComboBox *_cmbField, TComboBox *_text, TCheckBox *_sql, TCheckBox *_exc, TRadioGroup *_rgTypeT);

	myRequest();

	static void loadTemplates(decltype(mTemplates) _mTemplates) {mTemplates = _mTemplates;};

	static void saveTemplate(const String& _group, const String _name);
	static Request getTemplate(const String& _group, const String _name);

	static const decltype(mTemplates)& getTemplates(){return mTemplates;};
	static const std::map<std::string, Request>& getGroups(const String& _group);
	static void removeGroup(const String& _group);
	static void removeTemplate(const String& _group, const String _name);
	static void setTemplate(const String& _group, const String _name);
	static auto getInst(){return myRequest{};}
	String operator[](size_t i);
	static std::vector<String> groupsNames();
	static std::vector<String> groupTempaltes(const String& group_name);
};


std::map<std::string,std::map<std::string, Request>> myRequest::mTemplates{};
 TComboBox *myRequest::cmbField{};
 TComboBox *myRequest::cmbText{};
 TCheckBox *myRequest::chkSql{};
 TCheckBox *myRequest::chkExc{};
 TRadioGroup *myRequest::rgTypeT{};
//---------------------------------------------------------------------------
#endif
