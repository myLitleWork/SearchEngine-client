//---------------------------------------------------------------------------

#ifndef RecentComboHistoryH
#define RecentComboHistoryH
#include <deque>
#include <string>
#include <algorithm>
#include <Vcl.StdCtrls.hpp>    // для TComboBox

class RecentComboHistory {
public:
    RecentComboHistory(TComboBox* combo, size_t maxCount)
        : combo_(combo), maxCount_(maxCount) {}

    // Добавить строку (если её нет), сделать её первой (последней введённой)
    void add(const std::string& value) {
        // Удаляем если уже есть
        auto it = std::find(history_.begin(), history_.end(), value);
        if (it != history_.end())
            history_.erase(it);
        // Вставляем в начало
        history_.push_front(value);
        // Лишние удаляем с конца
        while (history_.size() > maxCount_)
            history_.pop_back();

        // Обновляем ComboBox.Items
        combo_->Items->BeginUpdate();
        combo_->Items->Clear();
        for (const auto& s : history_)
            combo_->Items->Add(s.c_str());
        combo_->Items->EndUpdate();
    }

    // Восстановить очередь (например, из файла/настройки)
    void set(const std::deque<std::string>& values) {
        history_ = values;
        while (history_.size() > maxCount_)
            history_.pop_back();
        combo_->Items->BeginUpdate();
        combo_->Items->Clear();
        for (const auto& s : history_)
            combo_->Items->Add(s.c_str());
        combo_->Items->EndUpdate();
    }

    const std::deque<std::string>& get() const { return history_; }

private:
    TComboBox* combo_;
    size_t maxCount_;
    std::deque<std::string> history_;
};

//---------------------------------------------------------------------------
#endif
