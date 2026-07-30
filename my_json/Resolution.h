//
// Created by user on 06.08.2023.
//

#pragma once
#include <iostream>
#include <vector>


struct Resolution
{
    std::string n_sht,date,time,id_r,text,chk;
    std::vector<size_t> executors;
    std::string getAddCommand();

    Resolution(const std::string& str);
    Resolution() = default;
};