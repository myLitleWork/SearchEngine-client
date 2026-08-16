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
end
