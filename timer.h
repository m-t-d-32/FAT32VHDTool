#ifndef FILEUTIL_H
#define FILEUTIL_H

#include <defines.h>
#include <QDateTime>

class Timer{
public:

    static QString getTime(unsigned date, unsigned time, unsigned mm10s){
        unsigned year = (date >> 9) + 1980;
        unsigned month = date >> 5 & 0xf;
        unsigned day = date & 0x1f;
        unsigned hour = time >> 11;
        unsigned minute = time >> 5 & 0x3f;
        unsigned mms = mm10s * 10;
        unsigned second = (time & 0x1f) * 2;
        second += mms / 1000;

        return QString::number(year) + "年" + QString::number(month)
                    + "月" + QString::number(day) + "日 "
                    + QString::number(hour) + "时"
                    + QString::number(minute) + "分"
                    + QString::number(second) + "秒";
    }

    static void setNowTime(unsigned & date, unsigned & time, unsigned & mm10s){
        QDateTime current_date_time = QDateTime::currentDateTime();
        date = 0;
        date |= current_date_time.date().year() - 1980;
        date <<= 4;
        date |= current_date_time.date().month();
        date <<= 5;
        date |= current_date_time.date().day();
        time = 0;
        time |= current_date_time.time().hour();
        time <<= 6;
        time |= current_date_time.time().minute();
        time <<= 5;
        time |= current_date_time.time().second() / 2;
        mm10s = 0;
        mm10s += current_date_time.time().msec() / 10;
    }

};

#endif // FILEUTIL_H
