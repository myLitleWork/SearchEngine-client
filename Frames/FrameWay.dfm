inherited frWay: TfrWay
  PixelsPerInch = 192
  object dbgWay: TDBGridEh
    Left = 0
    Top = 0
    Width = 1268
    Height = 113
    Margins.Left = 24
    Margins.Top = 24
    Margins.Right = 24
    Margins.Bottom = 24
    Align = alClient
    AutoFitColWidths = True
    DynProps = <>
    MinAutoFitWidth = 50
    ReadOnly = True
    SearchPanel.Location = splCellInplaceEh
    TabOrder = 0
    OnRowDetailPanelHide = dbgWayRowDetailPanelHide
    Columns = <
      item
        Alignment = taLeftJustify
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'number'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1053#1086#1084#1077#1088
        Width = 157
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'ddate'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1044#1072#1090#1072
        Width = 145
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'ttime'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1042#1088#1077#1084#1103
        Width = 202
      end
      item
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'kuda'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1050#1086#1084#1091
        Width = 286
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'type'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1058#1080#1087
        Visible = False
        Width = 70
      end
      item
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'ind'
        Footers = <>
        Visible = False
        Width = 70
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'str'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1057#1090#1088#1072#1085#1080#1094#1072
        Width = 181
      end
      item
        Alignment = taCenter
        CellButtons = <>
        DynProps = <>
        EditButtons = <>
        FieldName = 'pos'
        Footers = <>
        Title.Alignment = taCenter
        Title.Caption = #1057#1090#1088#1086#1082#1072
        Width = 163
      end>
    object RowDetailData: TRowDetailPanelControlEh
      Margins.Left = 24
      Margins.Top = 24
      Margins.Right = 24
      Margins.Bottom = 24
    end
  end
end
