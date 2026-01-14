object Form7: TForm7
  AlignWithMargins = True
  Left = 0
  Top = 0
  Align = alCustom
  Caption = #1055#1086#1080#1089#1082' '#1096#1090'.'
  ClientHeight = 576
  ClientWidth = 719
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poDesktopCenter
  OnClose = FormClose
  OnCreate = FormCreate
  OnShow = FormShow
  TextHeight = 15
  object DBGridEh1: TDBGridEh
    AlignWithMargins = True
    Left = 3
    Top = 3
    Width = 713
    Height = 570
    Align = alClient
    AllowedOperations = []
    AutoFitColWidths = True
    ColumnDefValues.Title.TitleButton = True
    DataSource = DataSource1
    DynProps = <>
    IndicatorOptions = [gioShowRowIndicatorEh, gioShowRecNoEh]
    MinAutoFitWidth = 100
    Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgAlwaysShowSelection, dgConfirmDelete, dgCancelOnExit]
    OptionsEh = [dghFixed3D, dghFrozen3D, dghData3D, dghResizeWholeRightPart, dghHighlightFocus, dghClearSelection, dghFitRowHeightToText, dghAutoSortMarking, dghMultiSortMarking, dghDialogFind, dghShowRecNo, dghColumnResize, dghColumnMove, dghAutoFitRowHeight, dghExtendVertLines]
    PopupMenu = PopupMenu1
    RowHeight = 4
    RowLines = 1
    SearchPanel.Enabled = True
    EditButtonsShowOptions = [sebShowOnlyForCurRowEh]
    SortLocal = True
    STFilter.InstantApply = True
    STFilter.Local = True
    STFilter.Location = stflInTitleFilterEh
    STFilter.Visible = True
    TabOrder = 0
    object RowDetailData: TRowDetailPanelControlEh
    end
  end
  object qry: TADOQuery
    Active = True
    ConnectionString = 
      'Provider=MSDASQL.1;Persist Security Info=False;Extended Properti' +
      'es="DSN=SQLite3 Datasource;Database=D:\F12\123.db;StepAPI=0;Sync' +
      'Pragma=NORMAL;NoTXN=0;Timeout=100000;ShortNames=0;LongNames=0;No' +
      'Creat=0;NoWCHAR=0;FKSupport=0;JournalMode=;OEMCP=0;LoadExt=;BigI' +
      'nt=0;JDConv=0;"'
    CursorType = ctStatic
    Filtered = True
    Parameters = <>
    SQL.Strings = (
      'select *'
      'from way')
    Left = 504
    Top = 304
    object qrynumber: TIntegerField
      Alignment = taLeftJustify
      DisplayLabel = #1053#1086#1084#1077#1088' '#1096#1090'.'
      DisplayWidth = 15
      FieldName = 'number'
      ReadOnly = True
    end
    object qryddate: TWideStringField
      DisplayLabel = #1044#1072#1090#1072' '#1074#1099#1076#1095#1080
      DisplayWidth = 15
      FieldName = 'ddate'
      ReadOnly = True
      Size = 255
    end
    object qryttime: TWideStringField
      DisplayLabel = #1042#1088#1077#1084#1103' '#1074#1099#1076#1072#1095#1080
      DisplayWidth = 15
      FieldName = 'ttime'
      ReadOnly = True
      Size = 255
    end
    object qrykuda: TWideStringField
      Alignment = taCenter
      DisplayLabel = #1050#1086#1084#1091' '#1074#1099#1076#1072#1085#1072
      DisplayWidth = 15
      FieldName = 'kuda'
      ReadOnly = True
      Size = 255
    end
    object qrytype: TIntegerField
      Alignment = taCenter
      DisplayLabel = #1042#1093'./'#1048#1089#1093'. (1/2)'
      DisplayWidth = 15
      FieldName = 'type'
      ReadOnly = True
    end
    object qryind: TAutoIncField
      Alignment = taLeftJustify
      DisplayWidth = 15
      FieldName = 'ind'
      ReadOnly = True
      Visible = False
    end
    object qrystr: TIntegerField
      DisplayLabel = 'C'#1090#1088#1072#1085#1080#1094#1072
      FieldName = 'str'
    end
    object qrypos: TIntegerField
      DisplayLabel = #8470'/'#1087'.'#1087'.'
      FieldName = 'pos'
    end
  end
  object ADODataDriverEh1: TADODataDriverEh
    DynaSQLParams.Options = []
    MacroVars.Macros = <>
    ProviderDataSet = qry
    SelectCommand.Parameters = <>
    UpdateCommand.Parameters = <>
    InsertCommand.Parameters = <>
    DeleteCommand.Parameters = <>
    GetrecCommand.Parameters = <>
    Left = 360
    Top = 352
  end
  object DataSource1: TDataSource
    DataSet = memTbl
    Left = 416
    Top = 256
  end
  object memTbl: TMemTableEh
    Active = True
    AggregatesActive = True
    FieldDefs = <
      item
        Name = 'number'
        DataType = ftInteger
        Precision = 15
      end
      item
        Name = 'ddate'
        DataType = ftWideString
        Size = 255
      end
      item
        Name = 'ttime'
        DataType = ftWideString
        Size = 255
      end
      item
        Name = 'kuda'
        DataType = ftWideString
        Size = 255
      end
      item
        Name = 'type'
        DataType = ftInteger
        Precision = 15
      end
      item
        Name = 'ind'
        DataType = ftAutoInc
        Precision = 15
      end
      item
        Name = 'str'
        DataType = ftInteger
        Precision = 15
      end
      item
        Name = 'pos'
        DataType = ftInteger
        Precision = 15
      end>
    Filtered = True
    FetchAllOnOpen = True
    IndexDefs = <>
    Params = <>
    DataDriver = ADODataDriverEh1
    StoreDefs = True
    Left = 208
    Top = 344
  end
  object PopupMenu1: TPopupMenu
    Left = 544
    Top = 408
    object N1: TMenuItem
      Caption = #1055#1077#1095#1072#1090#1100' '#1086#1090#1095#1077#1090#1072
      OnClick = N1Click
    end
  end
  object dbp: TPrintDBGridEh
    DBGridEh = DBGridEh1
    Options = []
    PageFooter.Font.Charset = DEFAULT_CHARSET
    PageFooter.Font.Color = clWindowText
    PageFooter.Font.Height = -12
    PageFooter.Font.Name = 'Segoe UI'
    PageFooter.Font.Style = []
    PageHeader.Font.Charset = DEFAULT_CHARSET
    PageHeader.Font.Color = clWindowText
    PageHeader.Font.Height = -12
    PageHeader.Font.Name = 'Segoe UI'
    PageHeader.Font.Style = []
    Units = MM
    Left = 512
    Top = 480
  end
end
