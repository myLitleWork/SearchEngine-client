//
// Created by user on 06.08.2023.
//

#include "Resolution.h"

#include <fstream>
//#include <boost/algorithm/string.hpp>

Resolution::Resolution(const std::string &str) {
    /*   {
         static int n = 0;
          std::ofstream resFile;
          std::locale::global(std::locale(""));
          auto delim = [](char c ){return c == ';';};
          std::vector<std::string> v;
          boost::split(v,str,delim);
          n_sht = (v[0]);
          date = (v[1]);
          time = (v[2]);
          id_r = (v[3]);




          text = v[4];
          resFile.open("res_" +std::to_string(n)+".txt");
          resFile << text;
          resFile.close();
          text = "res_" +std::to_string(n)+".txt";
          chk = (v[5]);
          n++;
      }*/
}

std::string Resolution::getAddCommand() {
    return ("INSERT INTO RESOLUTIONS (N_SHT, DDATE, TTIME, ID_R, TTEXT, CHK) VALUES ('" + n_sht + "','" + date + "','" + time + "','" + id_r + "','" + text + "','" + chk + "')");
}
