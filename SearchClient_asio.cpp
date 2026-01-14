//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop
#include <tchar.h>
//---------------------------------------------------------------------------
USEFORM("formMain.cpp", fMain);
USEFORM("formRes.cpp", fRes);
//USEFORM("Frames\MyFrameDbg.cpp", FrameDbg); /* TFrame: File Type */
//USEFORM("Frames\FramePdtv.cpp", frPdtv); /* TFrame: File Type */
//USEFORM("Frames\FrameWay.cpp", frWay); /* TFrame: File Type */
//USEFORM("Frames\FrameAttachments.cpp", frAttachments); /* TFrame: File Type */
//---------------------------------------------------------------------------
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
{
	try
	{
		Application->Initialize();
		Application->MainFormOnTaskBar = true;
		Application->CreateForm(__classid(TfMain), &fMain);
		Application->CreateForm(__classid(TfRes), &fRes);
        ReportMemoryLeaksOnShutdown = true;
		Application->Run();
	}
	catch (Exception &exception)
	{
		Application->ShowException(&exception);
	}
	catch (...)
	{
		try
		{
			throw Exception("");
		}
		catch (Exception &exception)
		{
			Application->ShowException(&exception);
		}
	}
	return 0;
}
//---------------------------------------------------------------------------
