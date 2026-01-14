object fRes: TfRes
  Left = 0
  Top = 0
  Caption = #1056#1077#1079#1086#1083#1102#1094#1080#1103
  ClientHeight = 624
  ClientWidth = 516
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poMainFormCenter
  OnClose = FormClose
  OnShow = FormShow
  TextHeight = 15
  object Panel1: TPanel
    Left = 0
    Top = 447
    Width = 516
    Height = 177
    Align = alBottom
    Caption = 'Panel1'
    ShowCaption = False
    TabOrder = 0
    ExplicitTop = 414
    ExplicitWidth = 502
    object Label1: TLabel
      AlignWithMargins = True
      Left = 4
      Top = 4
      Width = 508
      Height = 15
      Align = alTop
      Alignment = taCenter
      Caption = #1058#1077#1082#1090' '#1088#1077#1079#1086#1083#1102#1094#1080#1080
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Segoe UI'
      Font.Style = []
      ParentFont = False
      ExplicitWidth = 89
    end
    object memo: TMemo
      AlignWithMargins = True
      Left = 4
      Top = 25
      Width = 508
      Height = 99
      Align = alClient
      ScrollBars = ssVertical
      TabOrder = 0
      ExplicitWidth = 494
    end
    object bAdd: TButton
      AlignWithMargins = True
      Left = 4
      Top = 130
      Width = 508
      Height = 43
      Align = alBottom
      Caption = #1044#1086#1073#1072#1074#1080#1090#1100
      TabOrder = 1
      OnClick = bAddClick
      ExplicitWidth = 494
    end
  end
  object Panel2: TPanel
    AlignWithMargins = True
    Left = 3
    Top = 3
    Width = 158
    Height = 362
    Align = alLeft
    Caption = 'Panel2'
    ShowCaption = False
    TabOrder = 1
    ExplicitHeight = 329
    object Label2: TLabel
      AlignWithMargins = True
      Left = 4
      Top = 4
      Width = 150
      Height = 15
      Align = alTop
      Alignment = taCenter
      Caption = #1055#1086#1076#1095#1080#1085#1077#1085#1085#1099#1077
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Segoe UI'
      Font.Style = [fsUnderline]
      ParentFont = False
      ExplicitWidth = 78
    end
    object chkExcs: TCheckListBox
      AlignWithMargins = True
      Left = 4
      Top = 25
      Width = 150
      Height = 333
      Align = alClient
      ItemHeight = 17
      TabOrder = 0
      OnClick = chkExcsClick
      ExplicitHeight = 300
    end
  end
  object Panel3: TPanel
    AlignWithMargins = True
    Left = 167
    Top = 3
    Width = 178
    Height = 362
    Align = alLeft
    Caption = 'Panel3'
    ShowCaption = False
    TabOrder = 2
    ExplicitHeight = 329
    object Label3: TLabel
      AlignWithMargins = True
      Left = 4
      Top = 4
      Width = 170
      Height = 15
      Align = alTop
      Alignment = taCenter
      Caption = #1048#1089#1087#1086#1083#1085#1080#1090#1077#1083#1080
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Segoe UI'
      Font.Style = [fsUnderline]
      ParentFont = False
      ExplicitWidth = 75
    end
    object lstIsp: TListBox
      AlignWithMargins = True
      Left = 4
      Top = 25
      Width = 170
      Height = 333
      Align = alClient
      ItemHeight = 15
      MultiSelect = True
      TabOrder = 0
      OnClick = lstIspClick
      ExplicitHeight = 300
    end
  end
  object Panel4: TPanel
    AlignWithMargins = True
    Left = 351
    Top = 3
    Width = 162
    Height = 362
    Align = alRight
    Caption = 'Panel3'
    ShowCaption = False
    TabOrder = 3
    ExplicitLeft = 337
    ExplicitHeight = 329
    object Label4: TLabel
      AlignWithMargins = True
      Left = 4
      Top = 4
      Width = 154
      Height = 15
      Align = alTop
      Alignment = taCenter
      Caption = #1057#1086#1080#1089#1087#1086#1083#1085#1080#1090#1077#1083#1080
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -12
      Font.Name = 'Segoe UI'
      Font.Style = [fsUnderline]
      ParentFont = False
      ExplicitWidth = 88
    end
    object lstSoIsp: TListBox
      AlignWithMargins = True
      Left = 4
      Top = 25
      Width = 154
      Height = 333
      Align = alClient
      ItemHeight = 15
      MultiSelect = True
      TabOrder = 0
      OnClick = lstSoIspClick
      ExplicitHeight = 300
    end
  end
  object Panel5: TPanel
    AlignWithMargins = True
    Left = 3
    Top = 371
    Width = 510
    Height = 73
    Align = alBottom
    Caption = 'Panel5'
    ShowCaption = False
    TabOrder = 4
    ExplicitTop = 338
    ExplicitWidth = 496
    object rgIS: TRadioGroup
      AlignWithMargins = True
      Left = 4
      Top = 4
      Width = 154
      Height = 65
      Align = alLeft
      Caption = #1048#1089#1087'./'#1057#1086#1080#1089#1087'.'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -11
      Font.Name = 'Segoe UI'
      Font.Style = []
      ItemIndex = 0
      Items.Strings = (
        #1048#1089#1087#1086#1083#1085#1080#1090#1077#1083#1080
        #1057#1086#1080#1089#1087#1086#1083#1085#1080#1090#1077#1083#1080)
      ParentFont = False
      TabOrder = 0
    end
    object bAddRem: TButton
      AlignWithMargins = True
      Left = 164
      Top = 16
      Width = 174
      Height = 46
      Caption = #1044#1086#1073#1072#1074#1080#1090#1100'/'#1059#1073#1088#1072#1090#1100
      TabOrder = 1
      OnClick = bAddRemClick
    end
    object dp: TDatePicker
      AlignWithMargins = True
      Left = 352
      Top = 32
      Width = 166
      Height = 52
      Date = 45150.000000000000000000
      DateFormat = 'dd/mm/yyyy'
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clWindowText
      Font.Height = -16
      Font.Name = 'Segoe UI'
      Font.Style = [fsBold]
      TabOrder = 2
    end
    object chkSrok: TCheckBox
      AlignWithMargins = True
      Left = 352
      Top = 8
      Width = 154
      Height = 18
      Caption = #1057#1088#1086#1082' '#1080#1089#1087#1086#1083#1085#1077#1085#1080#1103' '#1076#1086':'
      TabOrder = 3
      OnClick = chkSrokClick
    end
  end
end
