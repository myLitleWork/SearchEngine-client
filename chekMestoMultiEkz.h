//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
#include <Data.DB.hpp>
#include <queue>
#include <regex>
#include <Data.Win.ADODB.hpp>
#include "System.SysUtils.hpp"
#ifndef chekMestoMultiEkzH
#define chekMestoMultiEkzH

struct tlg
{
	AnsiString podp, tlgn, dp, dt, krs, kolE, lists, nE, gr, vhn, ishn, dtvh, dtish, main_type, kuda;
	AnsiString file_dir;

	static inline int ind = 1;

	tlg(const AnsiString& ekz, int type, TADOQuery* qry)
	{
		auto get = [qry](const AnsiString& s)
		{
			auto ss = qry->FieldByName(s)->AsString;

			if(ss == "" && (ss == "PrilName1" ||  ss == "FFrom5" || ss.Length()> 50))
			{
				ss = "О направлении информации";
			}

			return ss;
		};

		auto getVhFromPrd = [qry] ()
		{
		   auto ss = qry->FieldByName("Blank")->AsString;

		   if( ss.Pos("Вх") > 0 )
				return ss;

		   return String{};
		};


		auto getIshFromPrm = [qry] ()
		{
		   auto ss = qry->FieldByName("GdeSht")->AsString;

		   if( ss.Pos("Исх") > 0 )
				return ss;

		   return String{};
		};

		dt = get("ddata");
		gr = get("sekretno");

		tlgn = get("Index");

        file_dir = get("DirectTo")+get("FileName");

		if(type == 0)
		{
			krs = get("PrilName1");
			vhn = "Вх.№ " + get("Index");
			ishn = getIshFromPrm();

			main_type = "Вх.";

			dtvh = dt;

			if(ishn != "")
				dtish =  dt;
		}
		else
		{
			krs = get("FFrom5");
			ishn = "Исх.№ " + get("Index");

			vhn = getVhFromPrd();

			dtish = dt;

			main_type = "Исх.";

			if(vhn != "")
				dtvh =  dt;
		}

		podp = get("PodpNo");
		dp = get("datapodp");
		kolE = get("Ekzempl");
		nE = ekz;
		lists = get("lists");

		krs = StringReplace(krs, "\"", "''", TReplaceFlags() << rfReplaceAll);

		qry->Close();
	}

	void addToAkt(TADOQuery* qry)
	{
	   qry->SQL->Clear();
	   qry->SQL->Add("insert into AKT (`index`, regno1, regno2, regno3, data1, data2, data3, podrazd, num, lists, noekz, sekretno) values (" + IntToStr(ind) + ", 'Подп.№ " +
										podp + "', '"+ vhn + "', '"+ ishn +"', '" + dp + "', '" + dtvh + "', '" + dtish + "', '" + krs + "', '" + IntToStr(ind) + "', '" + lists + "', '" + nE + "', '" + gr + "')" );
	   qry->ExecSQL();
	   ind++;
	}

};

class Mesto
{
	public:

	enum class MOD;

	private:

	static inline AnsiString bases_names[13];
	static inline const AnsiString archive_base = "ARCHIVE.db3";
	static inline AnsiString current_bases_dir{};
	const String vh_dir = "D:\\BASES\\";
	const String ish_dir = "D:\\BASES_PRD\\";

	TComponent* Owner;

	TADOConnection* con_base;
	TADOConnection* con_vh;
	TADOConnection* con_ish;
	TADOConnection* con_way;
	TADOQuery* qry_base;
	TADOQuery* qry_vh;
	TADOQuery* qry_ish;
	TADOQuery* qry_way;
	MOD work_mod;
	bool check_way;

	const static inline String ext = ".db3";




	std::queue<String> qBase;

    auto getCurBase()
	{
		return qBase.front();
	}

	bool find(const String& num)
	{
		try
		{
			qry_base->SQL->Text = "select * from ARCHIVE where `index` = '" + num  + "'";
			qry_base->Open();
		}
		catch(...)
		{
			return false;
			ShowMessage("error "+ current_bases_dir + " " + getCurBase() );
		}

		if(qry_base->RecordCount > 0)
			return true;

		return false;
	}

    auto getNextBase()
	{
		auto first = qBase.front();
		qBase.pop();
		qBase.push(first);
		return qBase.front();
	}

	bool findBase(const String& num)
	{
		if(find(num))
			return true;

		int n = 0;

		while(n < qBase.size())
		{
			n++;
			connectLocal(con_base, current_bases_dir + getNextBase());

			if(find(num))
				return true;
		}

		return false;
	}

	public:

	static String getNStr(const AnsiString str)
	{
		std::string s = str.c_str();
		std::regex r{"\\d{1,7}"};
		std::smatch m;

		if(std::regex_search(s,m,r))
		{
			std::string res = m.str().c_str();
			return String(res.c_str());
		}

		return "hz";
	}

    auto getQuery(const String& num)
	{
        if(!findBase(num))
			throw "Номер " + num +  "в базах" + (work_mod == MOD::VH ? "Входящих" : "Исходящих") + " шт - не найден";

		return qry_base;
	}

	enum class MOD : int
	{
		VH,
		ISH,
		UNKNOW
	};

	static void connectLocal(TADOConnection* con, const String& base)
	{
		static const  String sqlCSstart = "DSN=SQLite3 Datasource;Database=";
		static const  String sqlCSsend = ";StepAPI=0;SyncPragma=NORMAL;NoTXN=0;Timeout=100000;ShortNames=0;LongNames=0;NoCreat=0;NoWCHAR=0;FKSupport=0;JournalMode=;OEMCP=0;LoadExt=;BigInt=0;JDConv=0;PWD=";

		con->Connected = false;
		con->ConnectionString = sqlCSstart + base + sqlCSsend;
		con->Connected = true;
	}

	void setMod(MOD mod)
	{
		work_mod = mod;

		if(mod == MOD::VH)
			current_bases_dir = vh_dir;
		else if (mod == MOD::ISH)
			current_bases_dir = ish_dir;

		connectLocal(con_base, current_bases_dir + archive_base);
	}

	void checkWay(const String& num, const String kuda, String type = "")
	{
		if(type == "")
			type = work_mod == MOD::VH ? "1" : "2";


		qry_way->SQL->Text = "select * from way where number=" + num + " and type=" + type + " and kuda like('% л.%')";
		qry_way->Open();

		if(num == "7564")
		{
			int b = 1;
		}

		if(qry_way->RecordCount == 1)
		{
			auto old_gde = qry_way->FieldByName("kuda")->AsString;

			if(old_gde != kuda)
			{
			   qry_way->Edit();
			   qry_way->FieldByName("kuda")->AsString = kuda;
			   qry_way->Post();
			}

			qry_way->Close();
			return;

		} else if(qry_way->RecordCount > 1)
		{
			qry_way->Close();
			throw "Номер " + num + (work_mod == MOD::VH ? " Входящий" : " Исходящий") + " имеет больше одной записи о разноске в путях";
		}
		else
		{
			qry_way->Close();
			qry_way->SQL->Text = "insert into way (number, ddate, ttime, kuda, type, str, pos) values ('"+ num +"','" +
			Now().DateString()  + "','" +  Now().TimeString() + "','" + kuda + "','" + type + "','-','-')";
			qry_way->ExecSQL();
		}

		if(qry_way->RowsAffected != 1)
			throw (work_mod == MOD::VH ? "Вх. № " : "Исх. № " + num + " - отметка в путях не проставлена!");
	}

	Mesto(TComponent* _Owner, const String& god)
	{
        Owner = _Owner;

		String ms[13] = {"01","02","03","04","05","06","07","08","09","10","11","12","13"};

		for(int i = 0; i < 13; i++)
			bases_names[i] = "METH_BASES\\" + ms[i] + "-" + god + ext;

		con_base = new TADOConnection(_Owner);
		qry_base = new TADOQuery(_Owner);

		con_base->LoginPrompt = false;
		qry_base->Connection = con_base;

		work_mod = MOD::UNKNOW;

		qBase.push("ARCHIVE.db3");

		for(int i = 12; i > -1; i--)
		{
			AnsiString full_name = vh_dir +u"METH_BASES\\" + ms[i] + "-" + god + ext;
			if(FileExists(full_name))
				qBase.push(bases_names[i]);
		}
	}

	String getNewGde(AnsiString gde, const String& akt_date)
	{
		   auto sl = new TStringList();
		   sl->StrictDelimiter = true;

		   sl->Delimiter = ';';
		   sl->DelimitedText = gde;

		   if(sl->Count > 0)
		   {
			gde = "";
			sl->Strings[0] = akt_date;

			for(auto s:sl)
			if(s!="")
				gde += (s+";");
		   }

		   delete sl;

		return gde;
	}


	void makeCheckArchive(const String& num, String new_gde)
	{

		auto old_gde = qry_base->FieldByName("GdeSht")->AsString;

		if(work_mod == MOD::VH && qry_base->FieldByName("ekzempl")->AsInteger > 1)
			new_gde =  getNewGde(old_gde, new_gde);

        if(old_gde == new_gde)
			return;

		qry_base->Edit();
		qry_base->FieldByName("GdeSht")->AsString = new_gde;
		qry_base->Post();

		auto check = qry_base->FieldByName("GdeSht")->AsString;

		if(check != new_gde)
			throw (work_mod == MOD::VH ? "Вх. № " : "Исх. № " + num + " - разноска в базе не проставлена!");

	}

	bool makeCheck(const String& num, const String kuda)
	{
		if(!findBase(num))
			throw (work_mod == MOD::VH ? "Вх. № " : "Исх. № " + num + " - не найден в базах!");

		auto old_gde = qry_base->FieldByName("GdeSht")->AsString;

		if(num == "11673")
		{
			int b = 1;
		}

		if (work_mod == MOD::VH)
		{

			if (old_gde.Pos("Исх") > 0)
			{
				setMod(MOD::ISH);

				auto ish_num = getNStr(old_gde);

				if(!findBase(ish_num))
					throw "Исх. № " + ish_num + "не найден в базах!";

				makeCheckArchive(ish_num, kuda);

				if(check_way)
				{
					checkWay(ish_num, kuda);
					checkWay(num, kuda, "1");
				}
			}
			else
			{
				makeCheckArchive(num, kuda);

				if(check_way)
					checkWay(num, kuda);
			}

		}
		else if(work_mod == MOD::ISH )
		{
			makeCheckArchive(num, kuda);

			if(check_way)
			{
				checkWay(num, kuda);
				auto blank = qry_base->FieldByName("blank")->AsString.UpperCase();

				if (blank.Pos("ВХ") > 0)
					checkWay(getNStr(blank), kuda, "1");
			}
		}

		return true;
	}

	tlg getTlg(const String& num)
	{

		if(!findBase(num))
			throw "Номер " + num +  "в базах" + (work_mod == MOD::VH ? "Входящих" : "Исходящих") + " шт - не найден";

		return std::move(tlg{"1", static_cast<int>(work_mod), qry_base});
	}



	void connectWay(const String& way_base_dir)
	{
		check_way = true;

		con_way = new TADOConnection(Owner);
		qry_way = new TADOQuery(Owner);

		con_way->LoginPrompt = false;
		qry_way->Connection = con_way;

		connectLocal(con_way, way_base_dir);
	}
    String getGde1Ekz(String num)
	{
		if(!findBase(num))
			throw "Номер " + num +  "в базах" + (work_mod == MOD::VH ? "Входящих" : "Исходящих") + " шт - не найден";


		auto gde = qry_base->FieldByName("GdeSht")->AsString;

		if( !(work_mod == MOD::VH && qry_base->FieldByName("ekzempl")->AsInteger > 1) )
			return gde;

			auto sl = new TStringList();
			sl->Delimiter = ';';
			sl->StrictDelimiter = true;
			sl->Text = gde;

			if(sl->Count > 1)
				gde = sl->Strings[0];

			delete sl;



	   return gde;
	}
	~Mesto()
	{
		delete con_base;
		delete qry_base;
	}

};
#endif
