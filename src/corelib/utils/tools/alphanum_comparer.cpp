#include "alphanum_comparer.h"


namespace {
/*!
 *    \fn       compare - compare two strings
 *    \param    l - left sring.
 *    \param    r - right string.
 *
 *    \return
 *        l<r - result less than zero;
 *        l>r - result more than zero;
 *        l=r - result is zero.
 */
int compare(QString l, QString r)
{
    enum Mode { STRING, NUMBER } mode = STRING;

    int size;
    if (l.size() < r.size())
        size = l.size();
    else
        size = r.size();

    int i = 0;

    // runing throught both strings to position "size-1"
    while (i < size) {
        if (mode == STRING) {
            QChar lchar, rchar;
            bool ldigit, rdigit;
            while (i < size) {
                lchar = l.at(i);
                rchar = r.at(i);
                ldigit = lchar.isDigit();
                rdigit = rchar.isDigit();
                // if both simbols is numbers, using numbers state
                if (ldigit && rdigit) {
                    mode = NUMBER;
                    break;
                }
                if (ldigit)
                    return -1;
                if (rdigit)
                    return +1;
                // both simbols are letters
                if (lchar < rchar)
                    return -1;
                if (lchar > rchar)
                    return +1;
                // simbols are equal
                i++;
            }
        } else { // mode == NUMBER
            unsigned long long lnum = 0, rnum = 0;
            int li = i, ri = i; // local indexes
            int ld = 0, rd = 0; // numbers

            // make left number
            while (li < l.size()) {
                ld = l.at(li).digitValue();
                if (ld < 0)
                    break;
                lnum = lnum * 10 + ld;
                li++;
            }

            // make right number
            while (ri < r.size()) {
                rd = r.at(ri).digitValue();
                if (rd < 0)
                    break;
                rnum = rnum * 10 + rd;
                ri++;
            }

            long long delta = lnum - rnum;
            if (delta)
                return delta;

            // numbers are equal
            mode = STRING;
            if (li <= ri)
                i = li;
            else
                i = ri;
        }
    }
    // this is for situation when both strings to position "size-1" equals
    if (i < r.size())
        return -1;
    if (i < l.size())
        return +1;

    // strings are full equal
    return 0;
}
} // namespace

bool AlphanumComparer::lessThan(const QString& s1, const QString& s2)
{
    return compare(s1, s2) < 0;
}
