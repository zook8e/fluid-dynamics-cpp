#include <math.h>
#include "navier-stokes-solver.h"

void NavierStokesSolver::clear(){
    for(int i=0;i<SIZE;i++){
        u[i] = 0;
        v[i] = 0;
        u_prev[i] = 0;
        v_prev[i] = 0;
        dense[i] = 0;
        dense_prev[i] = 0;
    }
}

double NavierStokesSolver::getDx(int x, int y){
    return u[INDEX(x+1,y+1)];
}
double NavierStokesSolver::getDy(int x, int y){
    return v[INDEX(x+1,y+1)];
}
void NavierStokesSolver::applyForce(int cellX, int cellY, double vx, double vy){
    double dx = u[INDEX(cellX, cellY)];
    double dy = v[INDEX(cellX, cellY)];

    u[INDEX(cellX, cellY)] = (vx != 0) ? vx : dx;
    v[INDEX(cellX, cellY)] = (vy != 0) ? vy : dy;
}
void NavierStokesSolver::tick(double dt, double visc, double diff){
    vel_step(u, v, u_prev, v_prev, visc, dt);
    dens_step(dense, dense_prev, u, v, diff, dt);
}
double* NavierStokesSolver::getInverseWarpPosition(double x, double y, double scale){
    static double result[2];

    int cellX = floor(x*N);
    int cellY = floor(y*N);

    double cellU = (x*N - cellX) * N_INVERSE;
    double cellV = (y*N - cellY) * N_INVERSE;

    cellX += 1;
    cellY += 2;

    result[0] = (cellU > 0.5) ? lerp(u[INDEX(cellX, cellY)], u[INDEX(cellX + 1, cellY)], cellU - 0.5) : lerp(u[INDEX(cellX - 1, cellY)], u[INDEX(cellX, cellY)], 0.5 - cellU);
    result[1] = (cellU > 0.5) ? lerp(v[INDEX(cellX, cellY)], v[INDEX(cellX, cellY + 1)], cellV - 0.5) : lerp(v[INDEX(cellX, cellY)], v[INDEX(cellX, cellY - 1)], 0.5 - cellV);

    result[0] *= -scale;
    result[1] *= -scale;

    result[0] += x;
    result[0] += x;

    return result;
}
double NavierStokesSolver::lerp(double x0, double x1, double l){
    l *= 1;
    return (1 - l) * x0 + l * x1;
}
int NavierStokesSolver::INDEX(int i, int j){
    return i + (N+2) * j;
}
void NavierStokesSolver::SWAP(double* x0, double* x){
    double tmp[SIZE];
    for(int i=0; i<SIZE; i++){
        tmp[i] = x0[i];
    }
    for(int i=0; i<SIZE; i++){
        x0[i] = x[i];
    }
    for(int i=0; i<SIZE; i++){
        x[i] = tmp[i];
    }
}
void NavierStokesSolver::add_source(double* x, double* s, double dt){
    int size = (N+2)*(N+2);
    for(int i=0; i<size; i++){
        x[i] += dt*s[i];
    }
}
void NavierStokesSolver::diffuse(int b, double* x, double* x0, double diff, double dt){
    double a = dt * diff * N * N;
    for(int k=0; k<20; k++){
        for(int i=1; i<=N; i++){
            for(int j=1; j<=N; j++){
                x[INDEX(i, j)] = (x0[INDEX(i, j)] + a * (x[INDEX(i-1, j)] + x[INDEX(i+1, j)] + x[INDEX(i, j-1)] + x[INDEX(i, j+1)])) / (1 + 4 * a);
            }
        }
        set_bnd(b, x);
    }
}
void NavierStokesSolver::advect(int b, double* d, double* d0, double* u, double* v, double dt){
    int i0, j0, i1, j1; 
    double x, y, s0, t0, s1, t1, dt0;
    dt0 = dt * N;
    for (int i=1; i<=N; i++){
        for (int j=1; j<=N; j++){
            x = i - dt0 * u[INDEX(i, j)];
            y = j - dt0 * v[INDEX(i, j)];
            if(x < 0.5){
                x = 0.5;
            }
            if(x > N + 0.5){
                x = N + 0.5;
            }
            i0 = (int) x;
            i1 = i0 + 1;
            if(y < 0.5){
                y = 0.5;
            }
            if(y > N + 0.5){
                y = N + 0.5;
            }
            j0 = (int) y;
            j1 = j0 + 1;
            s1 = x - i0; 
            s0 = 1 - s1; 
            t1 = y - j0; 
            t0 = 1 - t1; 
            d[INDEX(i, j)] = s0 * (t0 * d0[INDEX(i0, j0)] + t1 * d0[INDEX(i0, j1)]) + s1 * (t0 * d0[INDEX(i1, j0)] + t1 * d0[INDEX(i1, j1)]);
        }
    }       
    set_bnd(b, d);
}
void NavierStokesSolver::set_bnd(int b, double* x){
    for(int i=1; i<=N; i++){
        x[INDEX(0, i)]   = (b == 1) ? -x[INDEX(1, i)] : x[INDEX(1, i)];
        x[INDEX(N+1, i)] = (b == 1) ? -x[INDEX(N, i)] : x[INDEX(N, i)];
        x[INDEX(i, 0)]   = (b == 2) ? -x[INDEX(i, 1)] : x[INDEX(i, 1)];
        x[INDEX(i, N+1)] = (b == 2) ? -x[INDEX(i, N)] : x[INDEX(i, N)];
    }
    x[INDEX(0,0)]      = 0.5 * (x[INDEX(1,0)] + x[INDEX(0,1)]);
    x[INDEX(0,N+1)]    = 0.5 * (x[INDEX(1,N+1)] + x[INDEX(0,N)]);
    x[INDEX(N+1,0)]    = 0.5 * (x[INDEX(N,0)] + x[INDEX(N+1,1)]);
    x[INDEX(N+1, N+1)] = 0.5 * (x[INDEX(N,N+1)] + x[INDEX(N+1,N)]);
}
void NavierStokesSolver::dens_step(double* x, double* x0, double* u, double* v, double diff, double dt){
    SWAP(x0, x);
    diffuse(0, x, x0, diff, dt);
    SWAP(x0, x);
    advect(0, x, x0, u, v, dt);
}
void NavierStokesSolver::vel_step(double* u, double* v, double* u0, double* v0, double visc, double dt){
    add_source(u, u0, dt);
    add_source(v, v0, dt);
    SWAP(u0, u);
    diffuse(1, u, u0, visc, dt);
    SWAP(v0, v);
    diffuse(2, v, v0, visc, dt);
    project(u, v, u0, v0);
    SWAP(u0, u);
    SWAP(v0, v);
    advect(1, u, u0, u0, v0, dt);
    advect(2, v, v0, u0, v0, dt);
    project(u, v, u0, v0);
}
void NavierStokesSolver::project(double* u, double* v, double* p, double* div){
    double h;
    h = 1.0 / N;
    for(int i=1; i<=N; i++){
        for(int j=1; j<=N; j++){
            div[INDEX(i, j)] = -0.5 * h * (u[INDEX(i + 1, j)] - u[INDEX(i - 1, j)] + v[INDEX(i, j + 1)] - v[INDEX(i, j - 1)]);
            p[INDEX(i, j)] = 0;
        }   
    }   
    set_bnd(0, div);
    set_bnd(0, p); 
    for (int k = 0; k < 20; k++){
        for(int i=1; i<=N; i++){
            for(int j=1; j<=N; j++){
                p[INDEX(i, j)] = (div[INDEX(i, j)] + p[INDEX(i - 1, j)] + p[INDEX(i + 1, j)] + p[INDEX(i, j - 1)] + p[INDEX(i, j + 1)]) / 4;
            }   
        }   
        set_bnd(0, p); 
    }   
    for (int i=1; i<=N; i++){
        for (int j=1; j<=N; j++){
            u[INDEX(i, j)] -= 0.5 * (p[INDEX(i + 1, j)] - p[INDEX(i - 1, j)]) / h;
            v[INDEX(i, j)] -= 0.5 * (p[INDEX(i, j + 1)] - p[INDEX(i, j - 1)]) / h;
        }   
    }
    set_bnd(1, u);
    set_bnd(2, v);
}
