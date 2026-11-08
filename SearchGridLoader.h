#pragma once
#include <FireDAC.Comp.Client.hpp>
#include "MemTableEh.hpp"
#include <chrono>

// Called by the UI timer. Neither FireDAC nor EhLib is used from a worker.
class SearchGridLoader {
public:
    SearchGridLoader(TFDQuery* query, TMemTableEh* table) : query_(query), table_(table),
        oldFetchMode_(query->FetchOptions->Mode), oldFetchAll_(table->FetchAllOnOpen) {}
    ~SearchGridLoader() {
        if (controlsHeld_) table_->EnableControls();
        table_->FetchAllOnOpen = oldFetchAll_;
        query_->FetchOptions->Mode = oldFetchMode_;
    }
    bool step() {
        if (!started_) {
            table_->Close();
            query_->Close();
            query_->FetchOptions->Mode = Firedac::Stan::Option::fmOnDemand;
            query_->SQL->Text = L"select * from res";
            query_->Open();
            table_->FetchAllOnOpen = false;
            table_->DisableControls();
            controlsHeld_ = true;
            table_->Open();
            started_ = true;
        }
        const auto start = std::chrono::steady_clock::now();
        do {
            if (table_->FetchRecords(100) == 0) {
                table_->First();
                table_->EnableControls();
                controlsHeld_ = false;
                return true;
            }
        } while (std::chrono::steady_clock::now() - start < std::chrono::milliseconds(15));
        return false;
    }
private:
    TFDQuery* query_;
    TMemTableEh* table_;
    Firedac::Stan::Option::TFDFetchMode oldFetchMode_;
    bool oldFetchAll_, started_ = false, controlsHeld_ = false;
};
