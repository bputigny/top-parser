#ifndef COORD_H
#define COORD_H

#include <IR.h>

class Coord {
    public:
        virtual ir::BinExpr div(const ir::Expr&) = 0;
        virtual ir::VectExpr grad(const ir::Expr&) = 0;
        virtual ir::VectExpr curl(const ir::Expr&) = 0;
};

class CartesianCoord : public Coord {
    protected:
        ir::Identifier x;
        ir::Identifier y;
        ir::Identifier z;

    public:
        CartesianCoord();

        virtual ir::BinExpr div(const ir::Expr&);
        virtual ir::VectExpr grad(const ir::Expr&);
        virtual ir::VectExpr curl(const ir::Expr&);

        ir::DiffExpr dX(const ir::ScalarExpr&);
        ir::DiffExpr dY(const ir::ScalarExpr&);
        ir::DiffExpr dZ(const ir::ScalarExpr&);
};

class SphericalCoord : public Coord {
    protected:
        ir::Identifier r;
        ir::Identifier theta;
        ir::Identifier phi;

    public:
        SphericalCoord();

        virtual ir::BinExpr div(const ir::Expr&);
        virtual ir::VectExpr grad(const ir::Expr&);
        virtual ir::VectExpr curl(const ir::Expr&);

        /// returns d(f)/dr
        ir::DiffExpr dR(const ir::ScalarExpr& f);

        /// returns d(f)/d theta
        ir::DiffExpr dTheta(const ir::ScalarExpr& f);

        /// returns d(f)/d phi
        ir::DiffExpr dPhi(const ir::ScalarExpr& f);
};

class SpheroidalCoord : public SphericalCoord {
    private:
        ir::Identifier zeta;
        ir::DiffExpr rz;
        ir::DiffExpr rzz;

    public:
        SpheroidalCoord();

        virtual ir::BinExpr div(const ir::Expr&);
        virtual ir::VectExpr grad(const ir::Expr&);
        virtual ir::VectExpr curl(const ir::Expr&);

        /// returns d(f)/dzeta
        ir::DiffExpr dZ(const ir::ScalarExpr& f);

        /// returns d(f)/d theta
        ir::DiffExpr dTheta(const ir::ScalarExpr& f);

        /// returns d(f)/d phi
        ir::DiffExpr dPhi(const ir::ScalarExpr& f);
};

#endif // COORD_H
