#ifndef QUAD_H
#define QUAD_H

#include <string>
#include <cstdio>

namespace firestep {

template<class T> class Quad { // a 4 element vector
    public:
        T value[4];
    public:
        Quad(T v1 = 0, T v2 = 0, T v3 = 0, T v4 = 0) {
            value[0] = v1;
            value[1] = v2;
            value[2] = v3;
            value[3] = v4;
        }
        string toString(const char *fmt = "[%d,%d,%d,%d]") const {
            char buf[50];
            snprintf(buf, sizeof(buf), fmt,
                     (T) value[0], (T) value[1], (T) value[2], (T) value[3]);
            return string(buf);
        };
		void clear() {
			value[0] = value[1] = value[2] = value[3] = 0;
		}
        Quad<T> operator+(Quad<T> that) {
            return Quad<T>(
                       value[0] + that.value[0],
                       value[1] + that.value[1],
                       value[2] + that.value[2],
                       value[3] + that.value[3]
                   );
        }
        Quad<T> operator-(Quad<T> that) {
            return Quad<T>(
                       value[0] - that.value[0],
                       value[1] - that.value[1],
                       value[2] - that.value[2],
                       value[3] - that.value[3]
                   );
        }
        Quad<T>& operator=(T that) {
            value[0] = that;
            value[1] = that;
            value[2] = that;
            value[3] = that;
            return *this;
        }
        Quad<T>& operator=(Quad<T> that) {
            value[0] = that.value[0];
            value[1] = that.value[1];
            value[2] = that.value[2];
            value[3] = that.value[3];
            return *this;
        }
        Quad<T> operator*(T that) {
            return Quad<T>(
                       value[0] * that,
                       value[1] * that,
                       value[2] * that,
                       value[3] * that
                   );
        }
        Quad<T>& operator*=(T that) {
            value[0] *= that;
            value[1] *= that;
            value[2] *= that;
            value[3] *= that;
            return *this;
        }
        Quad<T>& operator/=(T that) {
            value[0] /= that;
            value[1] /= that;
            value[2] /= that;
            value[3] /= that;
            return *this;
        }
        Quad<T>& operator+=(T that) {
            value[0] += that;
            value[1] += that;
            value[2] += that;
            value[3] += that;
            return *this;
        }
        Quad<T>& operator+=(Quad<T> that) {
            value[0] += that.value[0];
            value[1] += that.value[1];
            value[2] += that.value[2];
            value[3] += that.value[3];
            return *this;
        }
        bool operator==(Quad<T> that) {
            return value[0] == that.value[0] &&
                   value[1] == that.value[1] &&
                   value[2] == that.value[2] &&
                   value[3] == that.value[3];
        }
        bool operator!=(Quad<T> that) {
            return value[0] != that.value[0] ||
                   value[1] != that.value[1] ||
                   value[2] != that.value[2] ||
                   value[3] != that.value[3];
        }
};

template<class T1, class T2>
Quad<T1>& operator+=(Quad<T1> &qa, Quad<T2> qb) {
    qa.value[0] += (T1) qb.value[0];
    qa.value[1] += (T1) qb.value[1];
    qa.value[2] += (T1) qb.value[2];
    qa.value[3] += (T1) qb.value[3];
    return qa;
};

} // namespace firestep

#endif
