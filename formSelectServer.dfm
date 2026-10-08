object fSelectServer: TfSelectServer
  Left = 0
  Top = 0
  BorderIcons = [biSystemMenu]
  BorderStyle = bsDialog
  Caption = 'Select server'
  ClientHeight = 360
  ClientWidth = 460
  Color = clBtnFace
  DefaultMonitor = dmActiveForm
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  Position = poOwnerFormCenter
  Scaled = True
  OnAfterMonitorDpiChanged = FormAfterMonitorDpiChanged
  OnCreate = FormCreate
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 15
  object lbHint: TLabel
    AlignWithMargins = True
    Left = 16
    Top = 16
    Width = 428
    Height = 40
    Margins.Left = 16
    Margins.Top = 16
    Margins.Right = 16
    Margins.Bottom = 8
    Align = alTop
    Caption =
      'Several servers are configured for this year. Choose where to se' +
      'nd the database update command.'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -12
    Font.Name = 'Segoe UI'
    Font.Style = []
    ParentFont = False
    WordWrap = True
    ExplicitWidth = 280
  end
  object lstServers: TListBox
    AlignWithMargins = True
    Left = 16
    Top = 72
    Width = 428
    Height = 220
    Margins.Left = 16
    Margins.Top = 8
    Margins.Right = 16
    Margins.Bottom = 8
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -13
    Font.Name = 'Segoe UI'
    Font.Style = []
    ItemHeight = 20
    ParentFont = False
    TabOrder = 0
    OnClick = lstServersClick
    OnDblClick = lstServersDblClick
  end
  object pnBottom: TPanel
    Left = 0
    Top = 300
    Width = 460
    Height = 60
    Align = alBottom
    BevelOuter = bvNone
    Padding.Left = 16
    Padding.Top = 12
    Padding.Right = 16
    Padding.Bottom = 12
    ParentBackground = False
    TabOrder = 1
    object btnCancel: TButton
      AlignWithMargins = True
      Left = 335
      Top = 12
      Width = 109
      Height = 36
      Margins.Left = 8
      Margins.Top = 0
      Margins.Right = 0
      Margins.Bottom = 0
      Align = alRight
      Cancel = True
      Caption = 'Cancel'
      ModalResult = 2
      TabOrder = 1
    end
    object btnOk: TButton
      AlignWithMargins = True
      Left = 218
      Top = 12
      Width = 109
      Height = 36
      Margins.Left = 0
      Margins.Top = 0
      Margins.Right = 0
      Margins.Bottom = 0
      Align = alRight
      Caption = 'OK'
      Default = True
      TabOrder = 0
      OnClick = btnOkClick
    end
  end
  object pnOpis: TPanel
    Left = 0
    Top = 150
    Width = 460
    Height = 150
    Align = alBottom
    BevelOuter = bvNone
    TabOrder = 2
    Visible = False
    object lblOpisYear: TLabel
      Left = 16
      Top = 8
      Width = 170
      Height = 20
      Caption = 'Opis file year:'
    end
    object edtOpisYear: TEdit
      Left = 190
      Top = 4
      Width = 80
      Height = 25
      MaxLength = 4
      NumbersOnly = True
      ShowHint = True
      TabOrder = 0
      OnChange = OpisOptionsChange
    end
    object lblOpisPath: TLabel
      Left = 16
      Top = 38
      Width = 428
      Height = 52
      Anchors = [akLeft, akTop, akRight]
      AutoSize = False
      ShowHint = True
      WordWrap = True
    end
    object chkReplaceOpis: TCheckBox
      Left = 16
      Top = 96
      Width = 428
      Height = 40
      Anchors = [akLeft, akTop, akRight]
      Caption = 'Replace existing database after download'
      TabOrder = 1
      WordWrap = True
      OnClick = OpisOptionsChange
    end
  end
end
