object fMain: TfMain
  Scaled = True
  PixelsPerInch = 96
  Left = 429
  Top = 166
  AlphaBlend = True
  BiDiMode = bdLeftToRight
  BorderWidth = 1
  Caption = 'SearchClient_asio'
  ClientHeight = 883
  ClientWidth = 1352
  Color = clMoneyGreen
  DefaultMonitor = dmDesktop
  DoubleBuffered = True
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -13
  Font.Name = 'Segoe UI'
  Font.Style = []
  ParentBiDiMode = False
  Position = poDesigned
  StyleName = 'Windows'
  OnAfterMonitorDpiChanged = FormAfterMonitorDpiChanged
  OnClose = FormClose
  OnCreate = FormCreate
  OnResize = FormResize
  OnShow = FormShow
  TextHeight = 17
  object ggSaveProgress: TGauge
    AlignWithMargins = True
    Left = 3
    Top = 816
    Width = 1346
    Height = 33
    Align = alBottom
    Enabled = False
    Progress = 0
    ExplicitTop = 619
    ExplicitWidth = 966
  end
  object pnStatusBar: TPanel
    AlignWithMargins = True
    Left = 3
    Top = 852
    Width = 1346
    Height = 28
    Margins.Top = 0
    Align = alBottom
    BevelOuter = bvLowered
    ShowCaption = False
    TabOrder = 2
    ExplicitTop = 655
    ExplicitWidth = 966
    object lbStatus: TLabel
      AlignWithMargins = True
      Left = 5
      Top = 3
      Width = 1203
      Height = 22
      Margins.Left = 4
      Margins.Top = 2
      Margins.Right = 4
      Margins.Bottom = 2
      Align = alClient
      Caption = #1057#1090#1072#1090#1091#1089':'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clBlack
      Font.Height = -14
      Font.Name = 'Segoe UI'
      Font.Style = [fsBold]
      ParentFont = False
      Layout = tlCenter
      ExplicitLeft = 4
      ExplicitTop = 2
      ExplicitWidth = 48
      ExplicitHeight = 19
    end
    object lblUser: TLabel
      AlignWithMargins = True
      Left = 1216
      Top = 3
      Width = 125
      Height = 22
      Margins.Left = 4
      Margins.Top = 2
      Margins.Right = 4
      Margins.Bottom = 2
      Align = alRight
      Alignment = taRightJustify
      Caption = #1053#1077#1090' '#1072#1074#1090#1086#1088#1080#1079#1072#1094#1080#1080'!'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clMaroon
      Font.Height = -14
      Font.Name = 'Segoe UI'
      Font.Style = [fsBold]
      ParentFont = False
      Layout = tlCenter
      ExplicitLeft = 837
      ExplicitTop = 2
      ExplicitHeight = 19
    end
  end
  object Panel1: TPanel
    AlignWithMargins = True
    Left = 3
    Top = 3
    Width = 1346
    Height = 50
    Align = alTop
    Caption = 'Panel1'
    ShowCaption = False
    TabOrder = 0
    ExplicitWidth = 966
    object lblServerEndpoints: TLabel
      AlignWithMargins = True
      Left = 217
      Top = 4
      Width = 671
      Height = 30
      Align = alClient
      AutoSize = False
      EllipsisPosition = epEndEllipsis
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -14
      Font.Name = 'Segoe UI'
      Font.Style = [fsBold]
      ParentFont = False
      ShowAccelChar = False
      Layout = tlCenter
      ExplicitWidth = 505
      ExplicitHeight = 31
    end
    object Edit2: TEdit
      AlignWithMargins = True
      Left = 95
      Top = 4
      Width = 234
      Height = 35
      TabStop = False
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -24
      Font.Name = 'Courier New'
      Font.Style = [fsBold]
      Font.Quality = fqClearTypeNatural
      ParentFont = False
      TabOrder = 0
      Text = 'localhost'
      TextHint = 'Ip (192.168.10.10)'
      Visible = False
      StyleName = 'Windows'
    end
    object btnConnect: TButton
      AlignWithMargins = True
      Left = 1229
      Top = 5
      Width = 112
      Height = 28
      Margins.Left = 4
      Margins.Top = 4
      Margins.Right = 4
      Margins.Bottom = 4
      Align = alRight
      Caption = #1055#1086#1076#1082#1083#1102#1095#1077#1085#1080#1077
      Enabled = False
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Segoe UI'
      Font.Style = []
      ParentFont = False
      TabOrder = 1
      TabStop = False
      StyleName = 'Windows'
      OnClick = btnConnectClick
      ExplicitLeft = 849
    end
    object pnServerChoice: TPanel
      AlignWithMargins = True
      Left = 96
      Top = 4
      Width = 156
      Height = 42
      Margins.Left = 4
      Margins.Right = 4
      Align = alLeft
      BevelOuter = bvNone
      ShowCaption = False
      TabOrder = 2
      object lblServerGroup: TLabel
        Left = 0
        Top = 0
        Width = 156
        Height = 15
        Align = alTop
        AutoSize = False
        Caption = #1057#1077#1088#1074#1077#1088' / '#1075#1088#1091#1087#1087#1072
        FocusControl = cmbServerGroup
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -12
        Font.Name = 'Segoe UI'
        Font.Style = []
        ParentFont = False
        ShowAccelChar = False
        Layout = tlCenter
      end
      object cmbServerGroup: TComboBox
        AlignWithMargins = True
        Left = 0
        Top = 17
        Width = 156
        Height = 25
        Margins.Left = 0
        Margins.Top = 2
        Margins.Right = 0
        Margins.Bottom = 0
        Align = alTop
        Style = csDropDownList
        DropDownCount = 16
        Enabled = False
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'Segoe UI'
        Font.Style = [fsBold]
        Hint = #1042#1099#1073#1077#1088#1080#1090#1077' '#1086#1076#1080#1085' '#1089#1077#1088#1074#1077#1088' '#1080#1083#1080' '#1075#1088#1091#1087#1087#1091' '#1089#1077#1088#1074#1077#1088#1086#1074
        ParentFont = False
        ShowHint = True
        TabOrder = 0
        TabStop = False
        OnChange = cmbServerGroupChange
      end
    end
    object btnUpdate: TButton
      AlignWithMargins = True
      Left = 4
      Top = 4
      Width = 85
      Height = 30
      Align = alLeft
      Caption = #1054#1073#1085#1086#1074#1083#1077#1085#1080#1077'!'
      Enabled = False
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Segoe UI'
      Font.Style = [fsBold]
      ParentFont = False
      TabOrder = 3
      TabStop = False
      OnClick = btnUpdateClick
    end
    object btnDisconnect: TButton
      AlignWithMargins = True
      Left = 1109
      Top = 5
      Width = 112
      Height = 28
      Margins.Left = 4
      Margins.Top = 4
      Margins.Right = 4
      Margins.Bottom = 4
      Align = alRight
      Caption = #1054#1090#1082#1083#1102#1095#1077#1085#1080#1077
      Enabled = False
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Segoe UI'
      Font.Style = []
      ParentFont = False
      TabOrder = 4
      TabStop = False
      StyleName = 'Windows'
      OnClick = btnDisconnectClick
      ExplicitLeft = 729
    end
    object pnAuthChoice: TPanel
      AlignWithMargins = True
      Left = 895
      Top = 4
      Width = 206
      Height = 42
      Margins.Left = 4
      Margins.Right = 4
      Align = alRight
      BevelOuter = bvNone
      ShowCaption = False
      TabOrder = 5
      object lblAuthorization: TLabel
        Left = 0
        Top = 0
        Width = 206
        Height = 15
        Align = alTop
        AutoSize = False
        Caption = #1040#1074#1090#1086#1088#1080#1079#1072#1094#1080#1103
        FocusControl = cmbAuthUser
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -12
        Font.Name = 'Segoe UI'
        Font.Style = []
        ParentFont = False
        ShowAccelChar = False
        Layout = tlCenter
      end
      object cmbAuthUser: TComboBox
        AlignWithMargins = True
        Left = 0
        Top = 17
        Width = 206
        Height = 21
        Margins.Left = 0
        Margins.Top = 2
        Margins.Right = 0
        Margins.Bottom = 0
        Align = alTop
        Style = csDropDownList
        DropDownCount = 12
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -13
        Font.Name = 'Segoe UI'
        Font.Style = []
        ParentFont = False
        TabOrder = 0
        OnChange = cmbAuthUserChange
        OnCloseUp = cmbAuthUserCloseUp
      end
    end
  end
  object PageControl1: TPageControl
    AlignWithMargins = True
    Left = 3
    Top = 47
    Width = 1346
    Height = 763
    ActivePage = TabSheet1
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Segoe UI'
    Font.Style = [fsBold]
    MultiLine = True
    ParentFont = False
    TabOrder = 1
    StyleName = 'Windows'
    OnChange = PageControl1Change
    ExplicitWidth = 966
    ExplicitHeight = 566
    object TabSheet1: TTabSheet
      Caption = #1055#1086#1080#1089#1082
      object Label1: TLabel
        AlignWithMargins = True
        Left = 33
        Top = 86
        Width = 124
        Height = 28
        Caption = 'Relative index:'
        Enabled = False
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -20
        Font.Name = 'Segoe UI'
        Font.Style = []
        ParentFont = False
        Visible = False
      end
      object Panel9: TPanel
        ParentFont = False
        Font.Style = []
        Font.Height = -13
        Font.Name = 'Segoe UI'
        Font.Charset = DEFAULT_CHARSET
        ParentBackground = False
        Color = clMoneyGreen
        BevelOuter = bvNone
        Left = 0
        Top = 0
        Width = 1338
        Height = 128
        Align = alTop
        Caption = 'searchOptPanel'
        ShowCaption = False
        TabOrder = 0
        object grpSql: TGroupBox
          ParentFont = False
          Font.Style = [fsBold]
          Font.Height = -13
          Font.Name = 'Segoe UI'
          Font.Charset = DEFAULT_CHARSET
          Margins.Bottom = 4
          Margins.Right = 4
          Margins.Top = 4
          Margins.Left = 4
          AlignWithMargins = True
          Left = 4
          Top = 4
          Width = 150
          Height = 120
          Align = alLeft
          Caption = 'SQL'
          TabOrder = 0
          object chkExc: TCheckBox
            Margins.Bottom = 4
            Margins.Right = 6
            Margins.Top = 4
            Margins.Left = 6
            AlignWithMargins = True
            Left = 5
            Top = 48
            Width = 142
            Height = 24
            Align = alBottom
            Caption = #1058#1086#1095#1085#1086#1077' '#1089#1086#1074#1087#1072#1076#1077#1085#1080#1077
            Enabled = False
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            TabOrder = 0
          end
          object chkSQL: TCheckBox
            Margins.Bottom = 4
            Margins.Right = 6
            Margins.Top = 4
            Margins.Left = 6
            AlignWithMargins = True
            Left = 5
            Top = 22
            Width = 142
            Height = 24
            Align = alTop
            Caption = 'SQL'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            TabOrder = 1
            OnClick = chkSQLClick
          end
        end
        object rgTypeT: TRadioGroup
          Margins.Bottom = 4
          Margins.Right = 4
          Margins.Top = 4
          Margins.Left = 4
          AlignWithMargins = True
          Left = 162
          Top = 4
          Width = 72
          Height = 120
          Align = alLeft
          Caption = #1058#1080#1087
          Enabled = False
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe UI'
          Font.Style = [fsBold]
          ItemIndex = 0
          Items.Strings = (
            #1042#1093'.'
            #1048#1089#1093'.')
          ParentFont = False
          TabOrder = 1
        end
        object grpSqlParam: TGroupBox
          Margins.Bottom = 4
          Margins.Right = 4
          Margins.Top = 4
          Margins.Left = 4
          AlignWithMargins = True
          Left = 242
          Top = 4
          Width = 212
          Height = 120
          Align = alLeft
          Caption = #1055#1072#1088#1072#1084#1077#1090#1088#1099' '#1079#1072#1087#1088#1086#1089#1072
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe UI'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 2
          object grdParam: TGridPanel
            AlignWithMargins = True
            Left = 6
            Top = 20
            Width = 200
            Height = 94
            Margins.Left = 4
            Margins.Top = 2
            Margins.Right = 4
            Margins.Bottom = 4
            Align = alClient
            BevelOuter = bvNone
            Caption = 'grdParam'
            ColumnCollection = <
              item
                Value = 100.000000000000000000
              end>
            ControlCollection = <
              item
                Column = 0
                Control = Label2
                Row = 0
              end
              item
                Column = 0
                Control = Label3
                Row = 2
              end
              item
                Column = 0
                Control = cmbMaxReq
                Row = 3
              end
              item
                Column = 0
                Control = cmbField
                Row = 1
              end>
            RowCollection = <
              item
                Value = 20.000000000000000000
              end
              item
                Value = 30.000000000000000000
              end
              item
                Value = 20.000000000000000000
              end
              item
                Value = 30.000000000000000000
              end>
            ShowCaption = False
            TabOrder = 0
            TabStop = True
            object Label2: TLabel
              Layout = tlCenter
              Margins.Bottom = 0
              Margins.Right = 0
              Margins.Top = 0
              Margins.Left = 0
              AlignWithMargins = True
              Left = 3
              Top = 3
              Width = 160
              Height = 9
              Align = alClient
              Caption = #1055#1086#1083#1077':'
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Style = []
              ParentFont = False
            end
            object Label3: TLabel
              Layout = tlCenter
              Margins.Bottom = 0
              Margins.Right = 0
              Margins.Top = 0
              Margins.Left = 0
              AlignWithMargins = True
              Left = 3
              Top = 39
              Width = 160
              Height = 9
              Align = alClient
              Caption = #1052#1072#1082#1089#1080#1084#1091#1084' '#1086#1090#1074#1077#1090#1086#1074#58
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Style = []
              ParentFont = False
            end
            object cmbMaxReq: TComboBox
              Margins.Right = 0
              Margins.Top = 0
              Margins.Left = 0
              AlignWithMargins = True
              Left = 3
              Top = 54
              Width = 160
              Height = 20
              Margins.Bottom = 0
              Align = alClient
              Ctl3D = False
              Enabled = False
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Style = []
              ImeName = 'Russian'
              ItemIndex = 0
              ParentCtl3D = False
              ParentFont = False
              TabOrder = 0
              Text = '30'
              StyleName = 'Windows'
              Items.Strings = (
                '30'
                '60'
                '90'
                '300'
                '600'
                '10000'
                '9999999'
                #1041#1077#1079#32#1083#1080#1084#1080#1090#1072)
            end
            object cmbField: TComboBox
              Margins.Bottom = 0
              Margins.Right = 0
              Margins.Top = 0
              Margins.Left = 0
              AlignWithMargins = True
              Left = 3
              Top = 18
              Width = 160
              Height = 20
              Align = alClient
              Ctl3D = False
              DropDownCount = 11
              Enabled = False
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Style = []
              ImeName = 'Russian'
              ItemIndex = 0
              ParentCtl3D = False
              ParentFont = False
              TabOrder = 1
              Text = #1042#1093'/'#1080#1089#1093' '#8470
              StyleName = 'Windows'
              Items.Strings = (
                #1042#1093'/'#1080#1089#1093' '#8470
                #1055#1086#1076#1087#1080#1089#1085#1086#1081' '#8470
                #1050#1088'. '#1089#1086#1076#1077#1088#1078'.'
                #1050#1091#1076#1072'/'#1086#1090#1082#1091#1076#1072
                #1055#1086#1076#1087'./'#1080#1089#1087'.'
                #1058#1077#1083'. '#8470
                #1044#1072#1090#1072' '#1074#1093'/'#1080#1089#1093
                #1044#1072#1090#1072' '#1087#1086#1076#1087#1080#1089#1080
                #1050#1086#1083'-'#1074#1086' '#1087#1088#1080#1083'.'
                #1053#1072#1079#1074#1072#1085#1080#1077' '#1092#1072#1081#1083#1072' '#1087#1088#1080#1083'.'
                'SQL')
            end
          end
        end
        object GroupBox1: TGroupBox
          Margins.Bottom = 4
          Margins.Right = 4
          Margins.Top = 4
          Margins.Left = 4
          AlignWithMargins = True
          Left = 462
          Top = 4
          Width = 376
          Height = 120
          Align = alLeft
          Caption = #1064#1072#1073#1083#1086#1085#1099#32#1087#1086#1080#1089#1082#1072
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -13
          Font.Name = 'Segoe UI'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 3
          object Button1: TButton
            ParentFont = False
            Font.Style = []
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Charset = DEFAULT_CHARSET
            Margins.Bottom = 2
            Margins.Right = 6
            Margins.Top = 4
            Margins.Left = 6
            AlignWithMargins = True
            Left = 8
            Top = 54
            Width = 144
            Height = 26
            Align = alNone
            Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100#32#1096#1072#1073#1083#1086#1085
            TabOrder = 0
            OnClick = Button1Click
          end
          object Button3: TButton
            ParentFont = False
            Font.Style = []
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Charset = DEFAULT_CHARSET
            Margins.Bottom = 2
            Margins.Right = 6
            Margins.Top = 4
            Margins.Left = 6
            AlignWithMargins = True
            Left = 8
            Top = 22
            Width = 144
            Height = 26
            Align = alNone
            Caption = #1055#1086#1082#1072#1079#1072#1090#1100'/'#1089#1082#1088#1099#1090#1100
            TabOrder = 1
            OnClick = Button3Click
          end
        end
        object grpSaveOptions: TGroupBox
          ParentFont = False
          Font.Style = [fsBold]
          Font.Height = -13
          Font.Name = 'Segoe UI'
          Font.Charset = DEFAULT_CHARSET
          Margins.Bottom = 4
          Margins.Right = 4
          Margins.Top = 4
          AlignWithMargins = True
          Left = 846
          Top = 4
          Width = 376
          Height = 120
          Margins.Left = 4
          Align = alLeft
          Caption = #1057#1086#1093#1088#1072#1085#1077#1085#1080#1077#32#1076#1086#1082#1091#1084#1077#1085#1090#1086#1074
          TabOrder = 4
          object chkPril: TCheckBox
            AlignWithMargins = True
            Left = 8
            Top = 24
            Width = 120
            Height = 24
            Hint = 'Ctrl+Alt+P'
            Margins.Left = 6
            Margins.Top = 4
            Margins.Right = 6
            Margins.Bottom = 4
            Align = alNone
            Caption = #1055#1088#1080#1083#1086#1078#1077#1085#1080#1077
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            ParentShowHint = False
            ShowHint = True
            TabOrder = 0
          end
          object chkText: TCheckBox
            AlignWithMargins = True
            Left = 8
            Top = 58
            Width = 48
            Height = 24
            Hint = 'Ctrl+Alt+T'
            Margins.Left = 6
            Margins.Top = 4
            Margins.Right = 6
            Margins.Bottom = 4
            Align = alNone
            Caption = 'tlg'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            ParentShowHint = False
            ShowHint = True
            TabOrder = 1
            OnClick = chkTextClick
          end
          object chkDocx: TCheckBox
            AlignWithMargins = True
            Left = 64
            Top = 58
            Width = 64
            Height = 24
            Margins.Left = 6
            Margins.Top = 4
            Margins.Right = 6
            Margins.Bottom = 4
            Align = alNone
            Caption = 'docx'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            ParentShowHint = False
            ShowHint = False
            TabOrder = 2
            OnClick = chkDocxClick
          end

          object GroupBox3: TPanel
            Anchors = [akLeft, akTop, akRight]
            BevelOuter = bvNone
            ParentFont = False
            Font.Style = []
            Font.Height = -13
            Font.Name = 'Segoe UI'
            Font.Charset = DEFAULT_CHARSET
            Margins.Bottom = 4
            Margins.Right = 4
            Margins.Top = 4
            Margins.Left = 4
            AlignWithMargins = True
            Left = 140
            Top = 18
            Width = 228
            Height = 94
            Align = alNone
            Caption = ''
            TabOrder = 3
            object edtSaveDir: TEdit
              Anchors = [akLeft, akTop, akRight]
              Margins.Bottom = 4
              Margins.Right = 6
              Margins.Top = 4
              Margins.Left = 6
              AlignWithMargins = True
              Left = 8
              Top = 50
              Width = 212
              Height = 24
              AutoSize = True
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Style = []
              ParentFont = False
              ParentShowHint = False
              ShowHint = True
              TabOrder = 0
              TextHint = #1044#1074#1086#1081#1085#1086#1081' '#1097#1077#1083#1095#1086#1082' '#1076#1083#1103' '#1086#1095#1080#1089#1090#1082#1080
              OnDblClick = edtSaveDirDblClick
            end
            object btnSave: TButton
              ParentFont = False
              Font.Style = []
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Charset = DEFAULT_CHARSET
              Margins.Bottom = 4
              Margins.Right = 6
              Margins.Top = 4
              Margins.Left = 6
              AlignWithMargins = True
              Left = 8
              Top = 2
              Width = 128
              Height = 26
              Hint = 'Ctrl+S'
              Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100
              DropDownMenu = popSaveButton
              ParentShowHint = False
              ShowHint = True
              Style = bsSplitButton
              TabOrder = 1
              OnClick = btnSaveClick
            end
            object btnCancelSave: TButton
              ParentFont = False
              Font.Style = []
              Font.Height = -12
              Font.Name = 'Segoe UI'
              Font.Charset = DEFAULT_CHARSET
              AlignWithMargins = True
              Left = 138
              Top = 2
              Width = 82
              Height = 26
              Margins.Left = 6
              Margins.Top = 4
              Margins.Right = 6
              Margins.Bottom = 4
              Caption = #1054#1090#1084#1077#1085#1072
              Enabled = False
              TabOrder = 2
              OnClick = btnCancelSaveClick
            end
            object lblSaveFolder: TLabel
              Left = 8
              Top = 32
              Width = 212
              Height = 15
              AutoSize = False
              Caption = #1055#1072#1087#1082#1072#32#1076#1083#1103#32#1076#1086#1082#1091#1084#1077#1085#1090#1086#1074#58
              ParentFont = True
            end
          end
        end
        object grpResults: TGroupBox
          ParentFont = False
          Font.Style = [fsBold]
          Font.Height = -13
          Font.Name = 'Segoe UI'
          Font.Charset = DEFAULT_CHARSET
          Margins.Bottom = 4
          Margins.Right = 4
          Margins.Top = 4
          Margins.Left = 4
          AlignWithMargins = True
          Left = 1230
          Top = 4
          Width = 146
          Height = 120
          Align = alLeft
          Caption = #1056#1077#1079#1091#1083#1100#1090#1072#1090#1099
          TabOrder = 6
          object Button2: TButton
            Align = alTop
            Margins.Bottom = 2
            Margins.Right = 6
            Margins.Top = 4
            Margins.Left = 6
            AlignWithMargins = True
            Left = 8
            Top = 22
            Width = 130
            Height = 26
            Caption = #1069#1082#1089#1087#1086#1088#1090' '#1086#1090#1095#1077#1090#1072
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            PopupMenu = popExportButton
            TabOrder = 0
            OnClick = Button2Click
          end
          object btnEditColumns: TButton
            Align = alTop
            Margins.Bottom = 2
            Margins.Right = 6
            Margins.Top = 4
            Margins.Left = 6
            AlignWithMargins = True
            Left = 8
            Top = 54
            Width = 130
            Height = 26
            Caption = #1053#1072#1073#1086#1088' '#1082#1086#1083#1086#1085#1086#1082
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            TabOrder = 1
            OnClick = btnEditColumnsClick
          end
          object lbFilter: TLabel
            WordWrap = True
            AutoSize = False
            Align = alBottom
            Margins.Bottom = 2
            Margins.Right = 6
            Margins.Left = 6
            AlignWithMargins = True
            Left = 8
            Top = 86
            Width = 130
            Height = 28
            Margins.Top = 2
            Caption = #1042#1082#1083#1102#1095#1077#1085#1072' '#1092#1080#1083#1100#1090#1088#1072#1094#1080#1103'!'
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clMaroon
            Font.Height = -11
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            Visible = False
          end
          object edtLatency: TEdit
            Left = 8
            Top = 88
            Width = 40
            Height = 19
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -12
            Font.Name = 'Segoe UI'
            Font.Style = []
            ParentFont = False
            TabOrder = 2
            Text = '50'
            Visible = False
          end
        end
      end
      object pnSearchBody: TPanel
        Left = 0
        Top = 128
        Width = 1338
        Height = 603
        Align = alClient
        BevelOuter = bvNone
        ShowCaption = False
        TabOrder = 1
        object Splitter1: TSplitter
          AutoSnap = False
          MinSize = 220
          AlignWithMargins = True
          Left = 535
          Top = 3
          Width = 18
          Height = 626
          Color = clBtnFace
          ParentColor = False
        end
        object Panel7: TPanel
          Left = 556
          Top = 0
          Width = 782
          Height = 632
          Align = alClient
          Caption = 'Panel7'
          ShowCaption = False
          TabOrder = 1
          object ListBox1: TListBox
            AlignWithMargins = True
            Left = 4
            Top = 103
            Width = 774
            Height = 624
            Align = alClient
            Enabled = False
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -13
            Font.Name = 'Segoe UI'
            Font.Style = []
            ItemHeight = 17
            ParentFont = False
            TabOrder = 0
            Visible = False
            ExplicitWidth = 394
            ExplicitHeight = 427
          end
          object dbgEh: TDBGridEh
            Left = 1
            Top = 100
            Width = 780
            Height = 630
            Align = alClient
            AutoFitColWidths = True
            Border.ExtendedDraw = False
            Color = clMoneyGreen
            ColumnDefValues.EndEllipsis = True
            ColumnDefValues.Footer.ToolTips = True
            ColumnDefValues.Title.Color = clMoneyGreen
            ColumnDefValues.Title.EndEllipsis = True
            ColumnDefValues.Title.TitleButton = True
            ColumnDefValues.Title.ToolTips = True
            ColumnDefValues.ToolTips = True
            DataSource = srcSearch
            DynProps = <>
            Flat = True
            Font.Charset = DEFAULT_CHARSET
            Font.Color = clWindowText
            Font.Height = -13
            Font.Name = 'Segoe UI'
            Font.Style = []
            IndicatorOptions = [gioShowRowIndicatorEh, gioShowRecNoEh, gioShowRowselCheckboxesEh]
            MinAutoFitWidth = 50
            OddRowColor = clInfoBk
            Options = [dgTitles, dgIndicator, dgColumnResize, dgColLines, dgRowLines, dgTabs, dgRowSelect, dgAlwaysShowSelection, dgConfirmDelete, dgCancelOnExit, dgMultiSelect]
            OptionsEh = [dghFixed3D, dghFrozen3D, dghData3D, dghResizeWholeRightPart, dghHighlightFocus, dghClearSelection, dghAutoSortMarking, dghMultiSortMarking, dghEnterAsTab, dghDialogFind, dghShowRecNo, dghColumnResize, dghColumnMove, dghExtendVertLines]
            ParentFont = False
            ParentShowHint = False
            ReadOnly = True
            RowDetailPanel.Active = True
            RowDetailPanel.Height = 400
            SearchPanel.Enabled = True
            ShowHint = True
            SortLocal = True
            STFilter.InstantApply = True
            STFilter.Local = True
            STFilter.Location = stflInTitleFilterEh
            STFilter.Visible = True
            TabOrder = 1
            TitleParams.Color = clBlack
            TitleParams.FillStyle = cfstGradientEh
            TitleParams.Font.Charset = DEFAULT_CHARSET
            TitleParams.Font.Color = clWindowText
            TitleParams.Font.Height = -12
            TitleParams.Font.Name = 'Segoe UI'
            TitleParams.Font.Style = [fsBold]
            TitleParams.MultiTitle = True
            TitleParams.ParentFont = False
            TitleParams.SecondColor = clSkyBlue
            TitleParams.SortMarkerStyle = smstDefaultEh
            OnCellClick = dbgEhCellClick
            OnColWidthsChanged = dbgEhColWidthsChanged
            OnDrawColumnCell = dbgEhDrawColumnCell
            OnKeyPress = dbgEhKeyPress
            OnRowDetailPanelShow = dbgEhRowDetailPanelShow
            Columns = <
              item
                Alignment = taLeftJustify
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'num'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footer.EndEllipsis = True
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #8470
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 60
                WordWrap = False
              end
              item
                Alignment = taLeftJustify
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'tel_num'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #1058#1077#1083'. '#8470
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 60
                WordWrap = False
              end
              item
                Alignment = taCenter
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'date'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #1044#1072#1090#1072
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 60
                WordWrap = False
              end
              item
                Alignment = taLeftJustify
                CellButtons = <
                  item
                    Caption = '?'
                  end>
                DynProps = <>
                EditButtons = <>
                FieldName = 'podp_num'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #1055#1086#1076#1087#1080#1089#1085#1086#1081' '#1085#1086#1084#1077#1088
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 100
                WordWrap = False
              end
              item
                Alignment = taCenter
                AutoFitColWidth = False
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'date_podp'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 60
                TextEditing = False
                Title.Caption = #1044#1072#1090#1072' '#1087#1086#1076#1087'.'
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 100
                WordWrap = False
              end
              item
                Alignment = taLeftJustify
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'kr'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                Title.Caption = #1050#1088#1072#1090#1082#1086#1077' '#1089#1086#1076#1077#1088#1078#1072#1085#1080#1077
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 250
              end
              item
                Alignment = taCenter
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'isp'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #1048#1089#1087#1086#1083#1085#1080#1083'/ '#1087#1086#1076#1087#1080#1089#1072#1083
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 84
                WordWrap = False
              end
              item
                Alignment = taCenter
                CellButtons = <>
                Checkboxes = False
                DynProps = <>
                EditButtons = <>
                FieldName = 'type'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                MRUList.AutoAdd = False
                MRUList.Items.Strings = (
                  #1042#1093
                  #1048#1089#1093)
                TextEditing = False
                Title.Caption = #1058#1080#1087
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 55
                WordWrap = False
              end
              item
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'from_to'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #1054#1090#1082#1091#1076#1072'/'#1082#1091#1076#1072
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 100
                WordWrap = False
              end
              item
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'last_mesto'
                Footers = <>
                Title.Caption = #1042#1099#1076#1072#1085#1072
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 100
              end
              item
                CellButtons = <>
                Checkboxes = False
                DisplayFormat = '0.###,00###'
                DynProps = <>
                EditButtons = <>
                FieldName = 'rel'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Visible = False
                Width = 55
                WordWrap = False
              end
              item
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'dir'
                Font.Charset = DEFAULT_CHARSET
                Font.Color = clWindowText
                Font.Height = -12
                Font.Name = 'Segoe UI'
                Font.Style = []
                Footers = <>
                MaxWidth = 200
                MinWidth = 55
                TextEditing = False
                Title.Caption = #1044#1080#1088#1077#1082#1090#1086#1088#1080#1103' '#1092#1072#1081#1083#1072
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Visible = False
                Width = 100
                WordWrap = False
              end
              item
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'gde_sht'
                Footers = <>
                Title.Caption = #1056#1072#1079#1085#1086#1089#1082#1072
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 100
              end
              item
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'blank'
                Footers = <>
                Title.Caption = #1059#1095'.'#1083'.('#1048#1089#1093')/'#1044#1086#1083#1078#1085#1086#1089#1090#1100'('#1042#1093')'
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Visible = False
                Width = 100
              end
              item
                Alignment = taCenter
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'deleted'
                Footers = <>
                MaxWidth = 120
                MinWidth = 70
                TextEditing = False
                Title.Caption = #1057#1090#1072#1090#1091#1089
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 70
              end
              item
                Alignment = taCenter
                CellButtons = <>
                DynProps = <>
                EditButtons = <>
                FieldName = 'source_server_id'
                Footers = <>
                TextEditing = False
                Title.Font.Charset = DEFAULT_CHARSET
                Title.Font.Color = clWindowText
                Title.Font.Height = -12
                Title.Font.Name = 'Segoe UI'
                Title.Font.Style = [fsBold]
                Width = 70
              end>
            object RowDetailData: TRowDetailPanelControlEh
              Margins.Left = 24
              Margins.Top = 24
              Margins.Right = 24
              Margins.Bottom = 24
              object dbgMenu: TDBGridEh
                Left = 0
                Top = 0
                Width = 709
                Height = 398
                Align = alClient
                AutoFitColWidths = True
                DynProps = <>
                RowDetailPanel.Active = True
                TabOrder = 0
                OnRowDetailPanelHide = dbgMenuRowDetailPanelHide
                OnRowDetailPanelShow = dbgMenuRowDetailPanelShow
                Columns = <
                  item
                    CellButtons = <>
                    DynProps = <>
                    EditButtons = <>
                    FieldName = 'menu'
                    Footers = <>
                    Title.Caption = #1052#1077#1085#1102':'
                    Width = 60
                  end>
                object RowDetailData: TRowDetailPanelControlEh
                  Margins.Left = 6
                  Margins.Top = 6
                  Margins.Right = 6
                  Margins.Bottom = 6
                end
              end
            end
          end
          object pnShablons: TPanel
            AlignWithMargins = True
            Left = 4
            Top = 4
            Width = 134
            Height = 624
            Align = alLeft
            Caption = 'pnShablons'
            Color = clMoneyGreen
            ParentBackground = False
            ShowCaption = False
            TabOrder = 2
            Visible = False
            object Label5: TLabel
              AlignWithMargins = True
              Left = 4
              Top = 4
              Width = 126
              Height = 17
              Align = alTop
              Caption = #1043#1088#1091#1087#1087#1072' '#1096#1072#1073#1083#1086#1085#1086#1074':'
              ExplicitWidth = 118
            end
            object Label6: TLabel
              AlignWithMargins = True
              Left = 4
              Top = 294
              Width = 126
              Height = 17
              Align = alTop
              Caption = #1064#1072#1073#1083#1086#1085#1099':'
              ExplicitTop = 295
              ExplicitWidth = 67
            end
            object lstShGroups: TListBox
              AlignWithMargins = True
              Left = 4
              Top = 27
              Width = 126
              Height = 261
              Align = alTop
              ItemHeight = 17
              PopupMenu = PopupMenu2
              Sorted = False
              TabOrder = 0
              OnClick = lstShGroupsClick
            end
            object lstTemplates: TListBox
              AlignWithMargins = True
              Left = 4
              Top = 317
              Width = 126
              Height = 299
              Align = alClient
              ImeName = 'lstTemplates'
              ItemHeight = 17
              PopupMenu = popMenu
              Sorted = False
              TabOrder = 1
              OnClick = lstTemplatesClick
              OnDblClick = lstTemplatesDblClick
              ExplicitHeight = 102
            end
          end
        end
        object Panel8: TPanel
          ShowCaption = False
          AlignWithMargins = True
          Left = 3
          Top = 3
          Width = 526
          Height = 626
          Align = alLeft
          Caption = 'Panel8'
          Color = clMoneyGreen
          ParentBackground = False
          TabOrder = 0
          object Panel11: TPanel
            ShowCaption = False
            Left = 1
            Top = 1
            Width = 524
            Height = 624
            Align = alClient
            Caption = 'Panel11'
            TabOrder = 0
            object btnSearch: TButton
              AlignWithMargins = True
              Left = 4
              Top = 45
              Width = 376
              Height = 28
              Align = alTop
              Caption = #1055#1086#1080#1089#1082
              Enabled = False
              TabOrder = 1
              TabStop = False
              OnClick = btnSearchClick
              ExplicitTop = 71
            end
            object chkUpper: TCheckBox
              AlignWithMargins = True
              Left = 4
              Top = 79
              Width = 376
              Height = 17
              TabStop = False
              Align = alTop
              Caption = #1059#1095#1080#1090#1099#1074#1072#1090#1100' '#1088#1077#1075#1080#1089#1090#1088' '#1089#1080#1084#1074#1086#1083#1086#1074
              TabOrder = 3
              Visible = False
              ExplicitTop = 105
            end
            object cmbReq: TComboBox
              AlignWithMargins = True
              Left = 5
              Top = 5
              Width = 374
              Height = 33
              Margins.Left = 4
              Margins.Top = 4
              Margins.Right = 4
              Margins.Bottom = 4
              Align = alTop
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -19
              Font.Name = 'Segoe UI'
              Font.Style = []
              ParentFont = False
              TabOrder = 0
              TextHint = #1042#1074#1077#1076#1080#1090#1077' '#1079#1072#1087#1088#1086#1089
              OnChange = cmbReqChange
              OnKeyUp = cmbReqKeyUp
            end
            object pnTelegramFont: TPanel
              AlignWithMargins = True
              Left = 4
              Top = 102
              Width = 376
              Height = 28
              Align = alTop
              BevelOuter = bvNone
              ShowCaption = False
              Font.Charset = DEFAULT_CHARSET
              Font.Color = clWindowText
              Font.Height = -13
              Font.Name = 'Segoe UI'
              Font.Style = []
              ParentFont = False
              TabOrder = 2
              object chkOEM: TCheckBox
                AlignWithMargins = True
                Left = 0
                Top = 3
                Width = 100
                Height = 22
                Margins.Left = 0
                Align = alLeft
                TabStop = False
                Caption = 'OEM-866'
                Checked = True
                ParentFont = True
                State = cbChecked
                TabOrder = 0
              end
              object lblTelegramFontSize: TLabel
                AlignWithMargins = True
                Left = 249
                Top = 3
                Width = 58
                Height = 22
                Align = alRight
                AutoSize = False
                Caption = #1064#1088#1080#1092#1090':'
                FocusControl = cmbTelegramFontSize
                Layout = tlCenter
                ParentFont = True
              end
              object cmbTelegramFontSize: TComboBox
                AlignWithMargins = True
                Left = 313
                Top = 3
                Width = 60
                Height = 23
                Align = alRight
                Style = csDropDownList
                DropDownCount = 12
                Hint = #1056#1072#1079#1084#1077#1088' '#1096#1088#1080#1092#1090#1072' '#1090#1077#1082#1089#1090#1072' '#1074' '#1087#1091#1085#1082#1090#1072#1093'. '#1057#1086#1093#1088#1072#1085#1103#1077#1090#1089#1103' '#1072#1074#1090#1086#1084#1072#1090#1080#1095#1077#1089#1082#1080'.'
                ParentShowHint = False
                ParentFont = True
                ShowHint = True
                TabOrder = 1
                OnChange = cmbTelegramFontSizeChange
              end
            end
            object rchEdt: TRichEdit
              AlignWithMargins = True
              Left = 4
              Top = 136
              Width = 376
              Height = 486
              Align = alClient
              Enabled = False
              Font.Charset = RUSSIAN_CHARSET
              Font.Color = clWindowText
              Font.Height = -12
              Font.Name = 'Courier New'
              Font.Style = []
              ParentFont = False
              PopupMenu = popEdt
              ScrollBars = ssVertical
              TabOrder = 4
              OnClick = rchEdtClick
              ExplicitTop = 151
              ExplicitHeight = 274
            end
          end
        end
      end
    end
    object tbLoad: TTabSheet
      Margins.Left = 6
      Margins.Top = 6
      Margins.Right = 6
      Margins.Bottom = 6
      Caption = #1047#1072#1075#1088#1091#1079#1082#1072
      ImageIndex = 2
      OnShow = tbLoadShow
      object Panel10: TPanel
        AlignWithMargins = True
        Left = 6
        Top = 6
        Width = 507
        Height = 522
        Margins.Left = 6
        Margins.Top = 6
        Margins.Right = 6
        Margins.Bottom = 6
        Align = alLeft
        Caption = 'Panel10'
        TabOrder = 0
        ExplicitHeight = 529
        object Label7: TLabel
          AlignWithMargins = True
          Left = 7
          Top = 131
          Width = 493
          Height = 21
          Margins.Left = 6
          Margins.Top = 6
          Margins.Right = 6
          Margins.Bottom = 6
          Align = alTop
          Caption = #1060#1072#1081#1083#1099' '#1076#1083#1103' '#1079#1072#1075#1088#1091#1079#1082#1080':'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -16
          Font.Name = 'Segoe UI'
          Font.Style = [fsBold]
          ParentFont = False
          ExplicitWidth = 166
        end
        object btnLoadOnServerFiles: TButton
          AlignWithMargins = True
          Left = 7
          Top = 69
          Width = 493
          Height = 50
          Margins.Left = 6
          Margins.Top = 6
          Margins.Right = 6
          Margins.Bottom = 6
          Align = alTop
          Caption = #1047#1072#1075#1088#1091#1079#1080#1090#1100' '#1092#1072#1081#1083#1099' '#1085#1072' '#1086#1090#1087#1088#1072#1074#1082#1091
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -19
          Font.Name = 'Segoe UI'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 0
          OnClick = btnLoadOnServerFilesClick
          ExplicitWidth = 494
        end
        object memLogLoad: TMemo
          AlignWithMargins = True
          Left = 7
          Top = 482
          Width = 493
          Height = 33
          Margins.Left = 6
          Margins.Top = 6
          Margins.Right = 6
          Margins.Bottom = 6
          Align = alClient
          Lines.Strings = (
            'memLogLoad')
          ReadOnly = True
          TabOrder = 1
        end
        object lstFiles: TListBox
          AlignWithMargins = True
          Left = 7
          Top = 164
          Width = 493
          Height = 306
          Margins.Left = 6
          Margins.Top = 6
          Margins.Right = 6
          Margins.Bottom = 6
          Align = alTop
          ImeName = 'lst'
          ItemHeight = 18
          PopupMenu = popListFilesLoad
          TabOrder = 2
        end
        object btnAddFilesLoad: TButton
          AlignWithMargins = True
          Left = 7
          Top = 7
          Width = 493
          Height = 50
          Margins.Left = 6
          Margins.Top = 6
          Margins.Right = 6
          Margins.Bottom = 6
          Align = alTop
          Caption = #1044#1086#1073#1072#1074#1080#1090#1100' '#1092#1072#1081#1083#1099
          DisabledImageName = 'btnAddFilesLoad'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clWindowText
          Font.Height = -19
          Font.Name = 'Segoe UI'
          Font.Style = [fsBold]
          ParentFont = False
          TabOrder = 3
          OnClick = btnAddFilesLoadClick
          ExplicitWidth = 494
        end
      end
      object pnlRaznoska: TPanel
        AlignWithMargins = True
        Left = 522
        Top = 3
        Width = 327
        Height = 725
        Align = alLeft
        Caption = 'pnlRaznoska'
        ShowCaption = False
        TabOrder = 1
        ExplicitHeight = 535
        object btnSaveOpisBase: TButton
          AlignWithMargins = True
          Left = 3
          Top = 112
          Width = 322
          Height = 50
          Margins.Left = 2
          Margins.Top = 2
          Margins.Right = 2
          Margins.Bottom = 2
          Align = alTop
          Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100' '#1073#1072#1079#1091' '#1076#1083#1103' '#1086#1087#1080#1089#1077#1081
          TabOrder = 0
          OnClick = btnSaveOpisBaseClick
        end
        object btnLoadRazn: TButton
          AlignWithMargins = True
          Left = 3
          Top = 3
          Width = 322
          Height = 51
          Margins.Left = 2
          Margins.Top = 2
          Margins.Right = 2
          Margins.Bottom = 2
          Align = alTop
          Caption = #1047#1072#1075#1088#1091#1079#1080#1090#1100' '#1088#1072#1079#1085#1086#1089#1082#1091
          TabOrder = 1
          OnClick = btnLoadRaznClick
        end
        object btnAttachSave: TButton
          AlignWithMargins = True
          Left = 3
          Top = 58
          Width = 322
          Height = 50
          Margins.Left = 2
          Margins.Top = 2
          Margins.Right = 2
          Margins.Bottom = 2
          Align = alTop
          Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100' '#1087#1088#1080#1083#1086#1078#1077#1085#1080#1103
          TabOrder = 2
          OnClick = btnAttachSaveClick
        end
      end
    end
    object tbSettings: TTabSheet
      Caption = #1053#1072#1089#1090#1088#1086#1081#1082#1080
      ImageIndex = 2
      object tbcSettings: TTabControl
        AlignWithMargins = True
        Left = 3
        Top = 3
        Width = 952
        Height = 559
        Align = alClient
        TabOrder = 0
        OnChange = tbcSettingsChange
        object dbghSettings: TDBGridEh
          Left = 4
          Top = 6
          Width = 1324
          Height = 715
          Align = alClient
          AutoFitColWidths = True
          DataSource = srcSettings
          DynProps = <>
          TabOrder = 0
          object RowDetailData: TRowDetailPanelControlEh
            Margins.Left = 12
            Margins.Top = 12
            Margins.Right = 12
            Margins.Bottom = 12
          end
        end
      end
    end
  end
  object FileSaveDialog1: TFileSaveDialog
    FavoriteLinks = <>
    FileName = 'C:\Users\user\Documents\Embarcadero\Studio\Projects'
    FileTypes = <>
    Options = []
    Title = #1042#1074#1077#1076#1080#1090#1077' '#1080#1084#1103' '#1092#1072#1081#1083#1072' '#1076#1083#1103' '#1089#1086#1093#1088#1072#1085#1077#1085#1080#1103' '#1088#1077#1079#1091#1083#1100#1090#1072#1090#1086#1074' '#1079#1072#1087#1088#1086#1089#1072
    Left = 116
    Top = 688
  end
  object FileOpenDialog1: TFileOpenDialog
    FavoriteLinks = <>
    FileTypes = <>
    Options = []
    Left = 124
    Top = 816
  end
  object srcSearch: TDataSource
    DataSet = memTbl
    OnDataChange = srcSearchDataChange
    Left = 352
    Top = 432
  end
  object qryTelegi: TFDQuery
    Filtered = True
    Connection = ConnectionRes
    FetchOptions.AssignedValues = [evMode]
    FetchOptions.Mode = fmAll
    SQL.Strings = (
      'select *'
      'from res')
    Left = 504
    Top = 720
    object qryTeleginum: TIntegerField
      DisplayLabel = #1042#1093'/'#1048#1089#1093' '#8470
      FieldName = 'num'
      ReadOnly = True
    end
    object qryTelegidate: TDateField
      DisplayLabel = #1044#1072#1090#1072' '#1074#1093'/'#1080#1089#1093
      FieldName = 'date'
      ReadOnly = True
    end
    object qryTelegidate_podp: TDateField
      DisplayLabel = #1044#1072#1090#1072' '#1087#1086#1076#1087'.'
      FieldName = 'date_podp'
      ReadOnly = True
    end
    object qryTelegiisp: TWideStringField
      DisplayLabel = #1055#1086#1076#1087'./'#1080#1089#1087'.'
      FieldName = 'isp'
      ReadOnly = True
      Size = 2048
    end
    object qryTelegitype: TWideStringField
      DisplayLabel = #1042#1093'/'#1048#1089#1093
      DisplayWidth = 25
      FieldName = 'type'
      ReadOnly = True
      Size = 2048
    end
    object qryTelegipodp_num: TWideStringField
      DisplayLabel = #1055#1086#1076#1087'. '#8470
      FieldName = 'podp_num'
      ReadOnly = True
      Size = 2048
    end
    object qryTelegifrom_to: TWideStringField
      DisplayLabel = #1050#1091#1076#1072'/'#1086#1090#1082#1091#1076#1072
      FieldName = 'from_to'
      ReadOnly = True
      Size = 2048
    end
    object qryTelegirel: TFloatField
      DisplayLabel = #1056#1077#1083'-'#1090#1100
      FieldName = 'rel'
      ReadOnly = True
      DisplayFormat = '0.###,00###'
      currency = True
    end
    object qryTelegidir: TWideStringField
      DisplayLabel = #1055#1091#1090#1100' '#1082' '#1092#1072#1081#1083#1091
      FieldName = 'dir'
      ReadOnly = True
      Size = 2048
    end
    object qryTelegitel_num: TIntegerField
      DisplayLabel = #1058#1077#1083'. '#8470
      FieldName = 'tel_num'
      ReadOnly = True
    end
    object qryTelegikr: TWideStringField
      FieldName = 'kr'
      ReadOnly = True
      Size = 2048
    end
    object qryTelegipril_name: TWideStringField
      FieldName = 'pril_name'
      Size = 2048
    end
    object qryTelegipril_count: TWideStringField
      FieldName = 'pril_count'
      Size = 2048
    end
    object qryTelegilast_mesto: TWideStringField
      FieldName = 'last_mesto'
      Size = 2048
    end
    object qryTelegigde_sht: TWideStringField
      FieldName = 'gde_sht'
      Size = 2048
    end
    object qryTelegiblank: TWideStringField
      FieldName = 'blank'
      Size = 2048
    end
    object qryTelegideleted: TIntegerField
      DisplayLabel = #1057#1090#1072#1090#1091#1089
      FieldName = 'deleted'
      ReadOnly = True
    end
    object qryTelegisource_server_id: TWideStringField
      DisplayLabel = #1057#1077#1088#1074#1077#1088
      FieldName = 'source_server_id'
      ReadOnly = True
      Size = 2048
    end
  end
  object memTbl: TMemTableEh
    AggregatesActive = True
    FieldDefs = <
      item
        Name = 'num'
        DataType = ftInteger
        Precision = 15
      end
      item
        Name = 'date'
        DataType = ftDate
      end
      item
        Name = 'date_podp'
        DataType = ftDate
      end
      item
        Name = 'isp'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'type'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'podp_num'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'from_to'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'rel'
        DataType = ftFloat
        Precision = 15
      end
      item
        Name = 'dir'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'tel_num'
        DataType = ftInteger
        Precision = 15
      end
      item
        Name = 'kr'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'pril_name'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'pril_count'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'last_mesto'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'gde_sht'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'blank'
        DataType = ftWideString
        Size = 2048
      end
      item
        Name = 'deleted'
        DataType = ftInteger
      end
      item
        Name = 'source_server_id'
        DataType = ftWideString
        Size = 2048
      end>
    Filtered = True
    FetchAllOnOpen = True
    IndexDefs = <>
    Params = <>
    DataDriver = drvEh
    ReadOnly = True
    StoreDefs = True
    Left = 559
    Top = 476
    object memTblnum: TIntegerField
      FieldName = 'num'
    end
    object memTbldate: TDateField
      FieldName = 'date'
    end
    object memTbldate_podp: TDateField
      FieldName = 'date_podp'
    end
    object memTblisp: TWideStringField
      FieldName = 'isp'
      Size = 2048
    end
    object memTbltype: TWideStringField
      FieldName = 'type'
      Size = 2048
    end
    object memTblpodp_num: TWideStringField
      FieldName = 'podp_num'
      Size = 2048
    end
    object memTblfrom_to: TWideStringField
      FieldName = 'from_to'
      Size = 2048
    end
    object memTblrel: TFloatField
      FieldName = 'rel'
      Visible = False
    end
    object memTbldir: TWideStringField
      FieldName = 'dir'
      Size = 2048
    end
    object memTbltel_num: TIntegerField
      FieldName = 'tel_num'
    end
    object memTblkr: TWideStringField
      DisplayWidth = 100
      FieldName = 'kr'
      Size = 2048
    end
    object memTblpril_name: TWideStringField
      FieldName = 'pril_name'
      Size = 2048
    end
    object memTblpril_count: TWideStringField
      FieldName = 'pril_count'
      Size = 2048
    end
    object memTbllast_mesto: TWideStringField
      FieldName = 'last_mesto'
      Size = 2048
    end
    object memTblgde_sht: TWideStringField
      FieldName = 'gde_sht'
      Size = 2048
    end
    object memTblblank: TWideStringField
      FieldName = 'blank'
      Size = 2048
    end
    object memTbldeleted: TIntegerField
      FieldName = 'deleted'
      Visible = False
    end
    object memTblsource_server_id: TWideStringField
      DisplayLabel = #1057#1077#1088#1074#1077#1088
      FieldName = 'source_server_id'
      Size = 2048
    end
  end
  object ConnectionRes: TFDConnection
    Params.Strings = (
      'DriverID=SQLite')
    LoginPrompt = False
    Left = 594
    Top = 990
  end
  object ConnectionSettings: TFDConnection
    Params.Strings = (
      'DriverID=SQLite')
    LoginPrompt = False
    Left = 706
    Top = 990
  end
  object popDbg: TPopupMenu
    OnPopup = popDbgPopup
    Left = 1041
    Top = 644
  end
  object dbp: TPrintDBGridEh
    Options = [pghFitGridToPageWidth, pghOptimalColWidths]
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
    Left = 1588
    Top = 952
  end
  object qrySettings: TFDQuery
    BeforePost = qrySettingsBeforePost
    Connection = ConnectionSettings
    FetchOptions.AssignedValues = [evMode]
    FetchOptions.Mode = fmAll
    SQL.Strings = (
      'select * from ports')
    Left = 881
    Top = 988
  end
  object popMenu: TPopupMenu
    Left = 729
    Top = 532
    object N2: TMenuItem
      Caption = #1059#1076#1072#1083#1080#1090#1100
      OnClick = N2Click
    end
  end
  object PopupMenu2: TPopupMenu
    Left = 717
    Top = 786
    object N3: TMenuItem
      Caption = #1059#1076#1072#1083#1080#1090#1100
      OnClick = N3Click
    end
  end
  object frxReport: TfrxReport
    Version = '2022.1.3'
    DotMatrixReport = False
    IniFile = '\Software\Fast Reports'
    PreviewOptions.Buttons = [pbPrint, pbLoad, pbSave, pbExport, pbZoom, pbFind, pbOutline, pbPageSetup, pbTools, pbEdit, pbNavigator, pbExportQuick, pbCopy, pbSelection]
    PreviewOptions.Zoom = 1.000000000000000000
    PrintOptions.Printer = 'Default'
    PrintOptions.PrintOnSheet = 0
    ReportOptions.CreateDate = 45332.303099467600000000
    ReportOptions.LastChange = 45346.742345601850000000
    ScriptLanguage = 'PascalScript'
    ScriptText.Strings = (
      'begin'
      ''
      'end.')
    Left = 994
    Top = 456
    Datasets = <
      item
        DataSet = frxDBDataset1
        DataSetName = 'frxDBDataset1'
      end>
    Variables = <>
    Style = <>
    object Data: TfrxDataPage
      Height = 1000.000000000000000000
      Width = 1000.000000000000000000
    end
    object Page1: TfrxReportPage
      Orientation = poLandscape
      PaperWidth = 297.000000000000000000
      PaperHeight = 210.000000000000000000
      PaperSize = 9
      LeftMargin = 10.000000000000000000
      RightMargin = 10.000000000000000000
      TopMargin = 10.000000000000000000
      BottomMargin = 10.000000000000000000
      Frame.Typ = []
      MirrorMode = []
      object MasterData1: TfrxMasterData
        FillType = ftBrush
        FillGap.Top = 0
        FillGap.Left = 0
        FillGap.Bottom = 0
        FillGap.Right = 0
        Frame.Typ = []
        Height = 41.574830000000000000
        Top = 83.149660000000000000
        Width = 1046.929810000000000000
        DataSet = frxDBDataset1
        DataSetName = 'frxDBDataset1'
        RowCount = 0
        Stretched = True
        object frxDBDataset1date: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 173.876041760000000000
          Width = 75.590551180000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'date'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          GapX = 5.000000000000000000
          HAlign = haCenter
          Memo.UTF8W = (
            '[frxDBDataset1."date"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1podp_num: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 249.968770000000000000
          Width = 86.929141180000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'podp_num'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -11
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftRight, ftTop, ftBottom]
          GapX = 5.000000000000000000
          Memo.UTF8W = (
            '[frxDBDataset1."podp_num"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1date_podp: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 337.090200580000000000
          Width = 75.590551180000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'date_podp'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftRight, ftTop, ftBottom]
          GapX = 5.000000000000000000
          HAlign = haCenter
          Memo.UTF8W = (
            '[frxDBDataset1."date_podp"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1from_to: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 412.916567650000000000
          Width = 200.315090000000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'from_to'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -9
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftLeft, ftTop, ftBottom]
          GapX = 5.000000000000000000
          Memo.UTF8W = (
            '[frxDBDataset1."from_to"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1kr: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 612.283860000000000000
          Width = 283.464750000000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'kr'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          GapX = 5.000000000000000000
          Memo.UTF8W = (
            '[frxDBDataset1."kr"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1isp: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 895.748610000000000000
          Width = 151.181102360000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'isp'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          GapX = 5.000000000000000000
          HAlign = haCenter
          Memo.UTF8W = (
            '[frxDBDataset1."isp"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1num: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 98.267780000000000000
          Width = 75.590551180000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'num'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          GapX = 5.000000000000000000
          Memo.UTF8W = (
            '[frxDBDataset1."num"]')
          ParentFont = False
          WordBreak = True
        end
        object frxDBDataset1type: TfrxMemoView
          IndexTag = 1
          AllowVectorExport = True
          Left = 60.472480000000000000
          Width = 37.795251180000000000
          Height = 41.574803150000000000
          StretchMode = smMaxHeight
          DataField = 'type'
          DataSet = frxDBDataset1
          DataSetName = 'frxDBDataset1'
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = []
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            '[frxDBDataset1."type"]')
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo10: TfrxMemoView
          AllowVectorExport = True
          Width = 60.472480000000000000
          Height = 41.574830000000000000
          StretchMode = smMaxHeight
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            '[Line]')
          ParentFont = False
          VAlign = vaCenter
        end
      end
      object Header1: TfrxHeader
        FillType = ftBrush
        FillGap.Top = 0
        FillGap.Left = 0
        FillGap.Bottom = 0
        FillGap.Right = 0
        Frame.Typ = []
        Height = 41.574803150000000000
        Top = 18.897650000000000000
        Width = 1046.929810000000000000
        object Memo1: TfrxMemoView
          AllowVectorExport = True
          Left = 98.267780000000000000
          Width = 75.590551180000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1042#1093'/'#1080#1089#1093'. '#8470)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo2: TfrxMemoView
          AllowVectorExport = True
          Left = 173.858380000000000000
          Width = 75.590551180000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1044#1072#1090#1072' '#1074#1093'/'#1080#1089#1093)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo3: TfrxMemoView
          AllowVectorExport = True
          Left = 249.448980000000000000
          Width = 86.929141180000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -13
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1055#1086#1076#1087#1080#1089#1085#1086#1081' '#8470)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo4: TfrxMemoView
          AllowVectorExport = True
          Left = 336.378170000000000000
          Width = 75.590551180000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1044#1072#1090#1072' '#1087#1086#1076#1087#1080#1089#1080)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo5: TfrxMemoView
          AllowVectorExport = True
          Left = 411.968770000000000000
          Width = 200.315090000000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1040#1076#1088#1077#1089#1086#1074#1082#1072' - '#1082#1091#1076#1072'/'#1086#1090#1082#1091#1076#1072)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo6: TfrxMemoView
          AllowVectorExport = True
          Left = 612.283860000000000000
          Width = 283.464750000000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1050#1088#1072#1090#1082#1086#1077' '#1089#1086#1076#1077#1088#1078#1072#1085#1080#1077)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo7: TfrxMemoView
          AllowVectorExport = True
          Left = 895.748610000000000000
          Width = 151.181200000000000000
          Height = 41.574830000000000000
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1055#1086#1076#1087#1080#1089#1072#1083'/'#1080#1089#1087#1086#1083#1085#1080#1083
            '('#1074#1093'/'#1080#1089#1093')')
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo8: TfrxMemoView
          AllowVectorExport = True
          Left = 60.472480000000000000
          Width = 37.795251180000000000
          Height = 41.574830000000000000
          StretchMode = smMaxHeight
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #1058#1080#1087)
          ParentFont = False
          VAlign = vaCenter
        end
        object Memo9: TfrxMemoView
          AllowVectorExport = True
          Width = 60.472431180000000000
          Height = 41.574830000000000000
          StretchMode = smMaxHeight
          Font.Charset = DEFAULT_CHARSET
          Font.Color = clBlack
          Font.Height = -15
          Font.Name = 'Times New Roman'
          Font.Style = [fsBold]
          Frame.Typ = [ftLeft, ftRight, ftTop, ftBottom]
          HAlign = haCenter
          Memo.UTF8W = (
            #8470' '#1087'\'#1087)
          ParentFont = False
          VAlign = vaCenter
        end
      end
    end
  end
  object frxDBDataset1: TfrxDBDataset
    UserName = 'frxDBDataset1'
    CloseDataSource = False
    DataSet = memTbl
    BCDToCurrency = False
    DataSetOptions = []
    Left = 1826
    Top = 472
  end
  object myActiomList: TActionList
    Left = 1600
    Top = 88
    object actDoSearch: TAction
      Caption = 'actGoToSearch'
      ShortCut = 16454
      OnExecute = actDoSearchExecute
    end
    object actSetVh: TAction
      Caption = 'actSetVh'
      ShortCut = 32880
      OnExecute = actSetVhExecute
    end
    object actSetIsh: TAction
      Caption = 'actSetIsh'
      ShortCut = 32881
      OnExecute = actSetIshExecute
    end
    object actSetField: TAction
      Caption = 'actSetField'
      ShortCut = 32882
      OnExecute = actSetFieldExecute
    end
    object actSave: TAction
      Caption = 'actSave'
      ShortCut = 16467
      OnExecute = actSaveExecute
    end
    object actSetPril: TAction
      Caption = 'actSetPril'
      ShortCut = 49232
      OnExecute = actSetPrilExecute
    end
    object actSetText: TAction
      Caption = 'actSetText'
      ShortCut = 49236
      OnExecute = actSetTextExecute
    end
    object actShowRowPanel: TAction
      Caption = 'actShowRowPanel'
      ShortCut = 123
      OnExecute = actShowRowPanelExecute
    end
  end
  object drvEh: TDataSetDriverEh
    ProviderDataSet = qryTelegi
    Left = 289
    Top = 388
  end
  object tmLoadText: TTimer
    Interval = 400
    OnTimer = tmLoadTextTimer
    Left = 1056
    Top = 540
  end
  object popEdt: TPopupMenu
    Left = 66
    Top = 364
    object N6: TMenuItem
      Caption = #1055#1077#1095#1072#1090#1100
      ShortCut = 16464
      OnClick = N6Click
    end
  end
  object tmFilterEnableAlarm: TTimer
    OnTimer = tmFilterEnableAlarmTimer
    Left = 290
    Top = 984
  end
  object frxReport1: TfrxReport
    Version = '2022.1.3'
    DotMatrixReport = False
    IniFile = '\Software\Fast Reports'
    PreviewOptions.Buttons = [pbPrint, pbLoad, pbSave, pbExport, pbZoom, pbFind, pbOutline, pbPageSetup, pbTools, pbEdit, pbNavigator, pbExportQuick, pbCopy, pbSelection]
    PreviewOptions.Zoom = 1.000000000000000000
    PrintOptions.Printer = 'Default'
    PrintOptions.PrintOnSheet = 0
    ReportOptions.CreateDate = 45564.953478657410000000
    ReportOptions.LastChange = 45564.953478657410000000
    ScriptLanguage = 'PascalScript'
    ScriptText.Strings = (
      'begin'
      ''
      'end.')
    Left = 465
    Top = 948
    Datasets = <>
    Variables = <>
    Style = <>
  end
  object tmLoginIn: TTimer
    Interval = 1500
    OnTimer = tmLoginInTimer
    Left = 138
    Top = 180
  end
  object tmLoginOut: TTimer
    Enabled = False
    Interval = 500
    OnTimer = tmLoginOutTimer
    Left = 274
    Top = 180
  end
  object popListFilesLoad: TPopupMenu
    Left = 78
    Top = 937
    object delFileLoad: TMenuItem
      Caption = #1059#1076#1072#1083#1080#1090#1100
      OnClick = delFileLoadClick
    end
    object popItemClearFilesOnLoad: TMenuItem
      Caption = #1054#1095#1080#1089#1090#1080#1090#1100
      OnClick = popItemClearFilesOnLoadClick
    end
  end
  object srcSettings: TDataSource
    DataSet = qrySettings
    Left = 1023
    Top = 987
  end
  object DataSetProvider1: TDataSetProvider
    Left = 354
    Top = 846
  end
  object ClientDataSet1: TClientDataSet
    Aggregates = <>
    Params = <>
    Left = 1604
    Top = 460
  end
  object popExportButton: TPopupMenu
    Left = 642
    Top = 502
    object excel1: TMenuItem
      Caption = #1057#1086#1093#1088#1072#1085#1080#1090#1100' '#1074' excel'
      OnClick = excel1Click
    end
  end
  object frxXLSExport1: TfrxXLSExport
    UseFileCache = True
    ShowProgress = True
    OverwritePrompt = False
    DataOnly = False
    ExportEMF = True
    AsText = False
    Background = True
    FastExport = True
    PageBreaks = True
    EmptyLines = True
    SuppressPageHeadersFooters = False
    Left = 858
    Top = 518
  end
  object popSaveButton: TPopupMenu
    Left = 1322
    Top = 998
    object chkMakeDir: TMenuItem
      AutoCheck = True
      Caption = #1057#1076#1077#1083#1072#1090#1100' '#1086#1090#1076#1077#1083#1100#1085#1091#1102' '#1087#1072#1087#1082#1091
    end
  end
  object tmSaveProgress: TTimer
    Interval = 333
    OnTimer = tmSaveProgressTimer
    Left = 1586
    Top = 966
  end
end
