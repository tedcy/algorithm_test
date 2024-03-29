#include "/root/env/snippets/cpp/cpp_test_common.h"

#define MAX_ORDER 20

/* Enum for error messages */
enum curveFitERROR{
    ORDER_AND_NCOEFFS_DO_NOT_MATCH = -1,
    ORDER_INCORRECT = -2,
    NPOINTS_INCORRECT = -3
};

void cpyArray(double *src, double*dest, int n){
    for (int i = 0; i < n*n; i++){
        dest[i] = src[i];
    }
}

void subCol(double *mat, double* sub, uint8_t coln, uint8_t n){
    if (coln >= n) return;
    for (int i = 0; i < n; i++){
        mat[(i*n)+coln] = sub[i];
    }
}

/*Determinant algorithm taken from https://codeforwin.org/2015/08/c-program-to-find-determinant-of-matrix.html */
int trianglize(double **m, int n)
{
    int sign = 1;
    for (int i = 0; i < n; i++) {
        int max = 0;
        for (int row = i; row < n; row++)
            if (fabs(m[row][i]) > fabs(m[max][i]))
                max = row;
        if (max) {
            sign = -sign;
            double *tmp = m[i];
            m[i] = m[max], m[max] = tmp;
        }
        if (!m[i][i]) return 0;
        for (int row = i + 1; row < n; row++) {
            double r = m[row][i] / m[i][i];
            if (!r) continue;
            for (int col = i; col < n; col ++)
                m[row][col] -= m[i][col] * r;
        }
    }
    return sign;
}

double det(double *in, int n)
{
    double *m[n];
    m[0] = in;

    for (int i = 1; i < n; i++)
        m[i] = m[i - 1] + n;
    int sign = trianglize(m, n);
    if (!sign)
        return 0;
    double p = 1;
    for (int i = 0; i < n; i++)
        p *= m[i][i];
    return p * sign;
}
/*End of Determinant algorithm*/

//Raise x to power
double curveFitPower(double base, int exponent){
    if (exponent == 0){
        return 1;
    } else {
        double val = base;
        for (int i = 1; i < exponent; i++){
            val = val * base;
        }
        return val;
    }
}

int fitCurve (int order, vector<double> py, vector<double> &coeffs) {
    int i, j;
    double T[MAX_ORDER] = {0}; //Values to generate RHS of linear equation
    double S[MAX_ORDER*2+1] = {0}; //Values for LHS and RHS of linear equation
    double denom; //denominator for Cramer's rule, determinant of LHS linear equation
    double x, y;
    int nCoeffs = order + 1;

    vector<double> px(py.size()); //Generate X values, from 0 to n
    for (i=0; i< int(py.size()); i++){
        px[i] = i;
    }

    for (i=0; i < int(py.size()); i++) {//Generate matrix elements
        x = px[i];
        y = py[i];
        for (j = 0; j < (nCoeffs*2)-1; j++){
            S[j] += curveFitPower(x, j); // x^j iterated , S10 S20 S30 etc, x^0, x^1...
        }
        for (j = 0; j < nCoeffs; j++){
            T[j] += y * curveFitPower(x, j); //y * x^j iterated, S01 S11 S21 etc, x^0*y, x^1*y, x^2*y...
        }
    }

    double masterMat[nCoeffs*nCoeffs]; //Master matrix LHS of linear equation
    for (i = 0; i < nCoeffs ;i++){//index by matrix row each time
        for (j = 0; j < nCoeffs; j++){//index within each row
            masterMat[i*nCoeffs+j] = S[i+j];
        }
    }

    double mat[nCoeffs*nCoeffs]; //Temp matrix as det() method alters the matrix given
    cpyArray(masterMat, mat, nCoeffs);
    denom = det(mat, nCoeffs);
    cpyArray(masterMat, mat, nCoeffs);

    //Generate cramers rule mats
    for (i = 0; i < nCoeffs; i++){ //Temporary matrix to substitute RHS of linear equation as per Cramer's rule
        subCol(mat, T, i, nCoeffs);
        coeffs[nCoeffs-i-1] = det(mat, nCoeffs)/denom; //Coefficients are det(M_i)/det(Master)
        cpyArray(masterMat, mat, nCoeffs);
    }
    return 0;
}

int fitCurve (int order, vector<double> px, vector<double> py, vector<double> &coeffs) {
    int i, j;
    double T[MAX_ORDER] = {0}; //Values to generate RHS of linear equation
    double S[MAX_ORDER*2+1] = {0}; //Values for LHS and RHS of linear equation
    double denom; //denominator for Cramer's rule, determinant of LHS linear equation
    double x, y;
    int nCoeffs = order + 1;

    for (i=0; i < int(px.size()); i++) {//Generate matrix elements
        x = px[i];
        y = py[i];
        for (j = 0; j < (nCoeffs*2)-1; j++){
            S[j] += curveFitPower(x, j); // x^j iterated , S10 S20 S30 etc, x^0, x^1...
        }
        for (j = 0; j < nCoeffs; j++){
            T[j] += y * curveFitPower(x, j); //y * x^j iterated, S01 S11 S21 etc, x^0*y, x^1*y, x^2*y...
        }
    }

    double masterMat[nCoeffs*nCoeffs]; //Master matrix LHS of linear equation
    for (i = 0; i < nCoeffs ;i++){//index by matrix row each time
        for (j = 0; j < nCoeffs; j++){//index within each row
            masterMat[i*nCoeffs+j] = S[i+j];
        }
    }

    double mat[nCoeffs*nCoeffs]; //Temp matrix as det() method alters the matrix given
    cpyArray(masterMat, mat, nCoeffs);
    denom = det(mat, nCoeffs);
    cpyArray(masterMat, mat, nCoeffs);

    //Generate cramers rule mats
    for (i = 0; i < nCoeffs; i++){ //Temporary matrix to substitute RHS of linear equation as per Cramer's rule
        subCol(mat, T, i, nCoeffs);
        coeffs[nCoeffs-i-1] = det(mat, nCoeffs)/denom; //Coefficients are det(M_i)/det(Master)
        cpyArray(masterMat, mat, nCoeffs);
    }
    return 0;
} 
int main() {
    vector<double> vx = {0,0.25,0,5,0.75};
        vector<double> vy = {1,1.283,1.649,2.212,2.178};
    {
        vector<double> coeffs(4);
        fitCurve(2, vx, vy, coeffs);
        for (auto &v : coeffs) {
            cout << v << endl;
        }
    }
    {
        vector<double> coeffs(4);
        fitCurve(2, vy, coeffs);
        for (auto &v : coeffs) {
            cout << v << endl;
        }
    }
}
