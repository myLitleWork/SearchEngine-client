#pragma once
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.Dialogs.hpp>
#include <Vcl.StdCtrls.hpp>

// Native VCL drag threshold avoids turning an ordinary selection into a move.
class TTemplateListDrag : public TComponent {
    TListBox* list_;
    int sourceIndex_ = -1;
    void __fastcall Start(TObject*, TDragObject*&) { sourceIndex_ = list_ ? list_->ItemIndex : -1; }
    void __fastcall End(TObject*, TObject*, int, int) { sourceIndex_ = -1; }
    void __fastcall Over(TObject*, TObject* source, int x, int y, TDragState, bool& accept) {
        accept = list_ && source == list_ && sourceIndex_ >= 0 && sourceIndex_ < list_->Items->Count &&
            x >= 0 && x < list_->ClientWidth && y >= 0 && y < list_->ClientHeight;
    }
    void __fastcall Drop(TObject*, TObject* source, int x, int y) {
        bool accept = false;
        Over(list_, source, x, y, dsDragMove, accept);
        if (!accept) return;
        int target = list_->ItemAtPos(TPoint(x,y), true);
        if (target < 0) target = list_->Items->Count - 1;
        const int previous = sourceIndex_;
        if (target == previous) return;
        list_->Items->Move(previous, target);
        list_->ItemIndex = target;
        try {
            if (OnMoved) OnMoved(list_);
        } catch (...) {
            list_->Items->Move(target, previous);
            list_->ItemIndex = previous;
            throw;
        }
        sourceIndex_ = -1;
    }
protected:
    void __fastcall Notification(TComponent* component, TOperation operation) override {
        TComponent::Notification(component, operation);
        if (operation == opRemove && component == list_) list_ = nullptr;
    }
public:
    TNotifyEvent OnMoved = nullptr;
    __fastcall TTemplateListDrag(TComponent* owner, TListBox* list) : TComponent(owner), list_(list) {
        list_->FreeNotification(this);
        list_->Sorted = false;
        list_->DragMode = dmAutomatic;
        list_->OnStartDrag = Start;
        list_->OnEndDrag = End;
        list_->OnDragOver = Over;
        list_->OnDragDrop = Drop;
    }
    __fastcall ~TTemplateListDrag() {
        if (!list_) return;
        list_->OnStartDrag = nullptr;
        list_->OnEndDrag = nullptr;
        list_->OnDragOver = nullptr;
        list_->OnDragDrop = nullptr;
        list_->RemoveFreeNotification(this);
    }
};
