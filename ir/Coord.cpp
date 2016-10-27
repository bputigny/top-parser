#include <Coord.h>

CartesianCoord::CartesianCoord() : x("x"), y("y"), z("z") { }

ir::DiffExpr CartesianCoord::dX(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, x);
}

ir::DiffExpr CartesianCoord::dY(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, y);
}

ir::DiffExpr CartesianCoord::dZ(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, z);
}

ir::VectExpr CartesianCoord::grad(const ir::Expr& e) {
    try {
        const ir::ScalarExpr& se = dynamic_cast<const ir::ScalarExpr&>(e);

        ir::DiffExpr dx = this->dX(se);
        ir::DiffExpr dy = this->dY(se);
        ir::DiffExpr dz = this->dZ(se);
        return ir::VectExpr(dx, dy, dz);
    }
    catch (std::bad_cast) {
        err << "grad can only be applied to scalar expression\n";
        exit(EXIT_FAILURE);
    }
}

ir::BinExpr CartesianCoord::div(const ir::Expr &e) {
    try {
        const ir::VectExpr& ve = dynamic_cast<const ir::VectExpr&>(e);
        ir::DiffExpr dx = this->dX(*ve.getX());
        ir::DiffExpr dy = this->dY(*ve.getY());
        ir::DiffExpr dz = this->dZ(*ve.getZ());
        return dx + dy + dz;
    }
    catch (std::bad_cast) {
        err << "div can only be applied to vector expression\n";
        exit(EXIT_FAILURE);
    }
}

ir::VectExpr CartesianCoord::curl(const ir::Expr& e) {
    try {
        const ir::VectExpr& ve = dynamic_cast<const ir::VectExpr&>(e);

        ir::DiffExpr dzdy = this->dY(*ve.getZ());
        ir::DiffExpr dydz = this->dZ(*ve.getY());
        ir::DiffExpr dxdz = this->dZ(*ve.getX());
        ir::DiffExpr dzdx = this->dX(*ve.getZ());
        ir::DiffExpr dydx = this->dX(*ve.getY());
        ir::DiffExpr dxdy = this->dY(*ve.getX());

        return ir::VectExpr(
                dzdy - dydz,
                dxdz - dzdx,
                dydx - dxdy);
    }
    catch (std::bad_cast) {
        err << "grad can only be applied to scalar expression\n";
        exit(EXIT_FAILURE);
    }
}

SphericalCoord::SphericalCoord() : r("r"), theta("theta"), phi("phi") { }

ir::DiffExpr SphericalCoord::dR(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, r);
}

ir::DiffExpr SphericalCoord::dTheta(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, theta);
}

ir::DiffExpr SphericalCoord::dPhi(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, phi);
}

ir::BinExpr SphericalCoord::div(const ir::Expr& e) {
    // 1/r^2 d(r^2 Vr)/dr +
    // 1/(r sin(theta)) d(Vt sin(theta)) / dtheta +
    // 1/(r sin(theta)) d(Vp)/dphi
    try {
        const ir::VectExpr& v = dynamic_cast<const ir::VectExpr&>(e);
        ir::ScalarExpr& vr = *v.getX();
        ir::ScalarExpr& vt = *v.getY();
        ir::ScalarExpr& vp = *v.getZ();
        ir::BinExpr r2 = r^2;
        ir::BinExpr invR2 = 1 / r2;
        ir::BinExpr r2vr = r2 * vr;
        ir::DiffExpr dr2vrdr = this->dR(r2vr);
        ir::BinExpr rsint = r * ir::sin(theta);

        ir::BinExpr r = 1/r2 * dR(r2 * vr);
        ir::BinExpr t = (1/rsint) * dTheta(vt * ir::sin(theta));
        ir::BinExpr p = (1/rsint) * dPhi(vp);

        return r + t + p;

    }
    catch (std::bad_cast) {
        err << "div can only be applied to vector expression\n";
        exit(EXIT_FAILURE);
    }
}

ir::VectExpr SphericalCoord::grad(const ir::Expr& e) {
    try {
        const ir::ScalarExpr& s = dynamic_cast<const ir::ScalarExpr&>(e);
        ir::DiffExpr gr = this->dR(s);
        ir::BinExpr gt = 1/r * this->dTheta(s);
        ir::BinExpr gp = 1/(r*ir::sin(theta)) * this->dPhi(s);
        return ir::VectExpr(gr, gt, gp);
    }
    catch (std::bad_cast) {
        err << "grad can only be applied to scalar expression\n";
        exit(EXIT_FAILURE);
    }
}

ir::VectExpr SphericalCoord::curl(const ir::Expr& e) {
    try {
        const ir::VectExpr& v = dynamic_cast<const ir::VectExpr&>(e);
        ir::ScalarExpr& vr = *v.getX();
        ir::ScalarExpr& vt = *v.getY();
        ir::ScalarExpr& vp = *v.getZ();

        ir::FuncCall sint = ir::sin(theta);
        ir::BinExpr rsint = r * ir::sin(theta);

        return ir::VectExpr(
                1/rsint * (dTheta(vp*sint) - dPhi(vt)),
                1/r * (1/sint * dPhi(vr) - dR(r*vp)),
                1/r * (dR(r*vt) - dTheta(vr)));
    }
    catch (std::bad_cast) {
        err << "curl can only be applied to vector expression\n";
        exit(EXIT_FAILURE);
    }
}

SpheroidalCoord::SpheroidalCoord() : SphericalCoord(), zeta("zeta"),
    rz(r, zeta), rzz(rz, zeta) { }

ir::DiffExpr SpheroidalCoord::dZ(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, zeta);
}

ir::DiffExpr SpheroidalCoord::dTheta(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, theta);
}

ir::DiffExpr SpheroidalCoord::dPhi(const ir::ScalarExpr& s) {
    return ir::DiffExpr(s, phi);
}

ir::BinExpr SpheroidalCoord::div(const ir::Expr&) {
    err << "div in spheroidal coordinate not yet implemented\n";
    exit(EXIT_FAILURE);
}

ir::VectExpr SpheroidalCoord::grad(const ir::Expr& e) {
    try {
        const ir::ScalarExpr& s = dynamic_cast<const ir::ScalarExpr&>(e);
        ir::DiffExpr gz = this->dZ(s);
        ir::DiffExpr gt = this->dTheta(s);
        ir::DiffExpr gp = this->dPhi(s);
        return ir::VectExpr(gz, gt, gp);
    }
    catch (std::bad_cast) {
        err << "grad can only be applied to scalar expression\n";
        exit(EXIT_FAILURE);
    }
}

ir::VectExpr SpheroidalCoord::curl(const ir::Expr&) {
    err << "curl in spheroidal coordinate not yet implemented\n";
    exit(EXIT_FAILURE);
}
