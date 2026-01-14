inherited frPdtv: TfrPdtv
  PixelsPerInch = 192
  object dbgPdtv: TDBGridEh
    Left = 0
    Top = 0
    Width = 1280
    Height = 225
    Margins.Left = 24
    Margins.Top = 24
    Margins.Right = 24
    Margins.Bottom = 24
    Align = alClient
    DynProps = <>
    MinAutoFitWidth = 50
    ReadOnly = True
    RowDetailPanel.Active = True
    RowDetailPanel.Height = 100
    SearchPanel.Location = splCellInplaceEh
    TabOrder = 0
    OnRowDetailPanelShow = dbgPdtvRowDetailPanelShow
    Columns = <
      item
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'kuda'
        Footers = <>
        MaxWidth = 500
        MinWidth = 300
        Title.Caption = #1040#1076#1088#1077#1089#1072#1090
        Width = 300
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'file'
        Footers = <>
        MaxWidth = 85
        MinWidth = 30
        Title.Alignment = taCenter
        Title.Caption = #1060#1072#1081#1083
        Width = 85
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'ddate_time'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1044#1072#1090#1072'_'#1074#1088#1077#1084#1103
        Width = 182
      end
      item
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'dir'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1044#1080#1088#1088#1077#1082#1090#1086#1088#1080#1103
        Width = 367
      end>
    object RowDetailData: TRowDetailPanelControlEh
      Margins.Left = 24
      Margins.Top = 24
      Margins.Right = 24
      Margins.Bottom = 24
      object memPdtv: TMemo
        Left = 0
        Top = 0
        Width = 917
        Height = 98
        Margins.Left = 6
        Margins.Top = 6
        Margins.Right = 6
        Margins.Bottom = 6
        Align = alClient
        ScrollBars = ssVertical
        TabOrder = 0
        ExplicitWidth = 616
      end
    end
  end
end
