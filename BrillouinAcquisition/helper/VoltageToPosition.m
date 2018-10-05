function [x, y] = VoltageToPosition(Ux, Uy, distortion)

    R_old = sqrt(Ux.^2 + Uy.^2);
    R_new = distortion(4) * R_old + distortion(5) * R_old.^2 + distortion(6) * R_old.^3 + distortion(7) * R_old.^4;
    
    rho = distortion(3);
    
    Ux_rot = Ux * cos(rho) - Uy * sin(rho);
    Uy_rot = Ux * sin(rho) + Uy * cos(rho);
    
    if (R_old == 0)
        x = Ux_rot;
        y = Uy_rot;
    else
        x = Ux_rot./R_old .* R_new;
        y = Uy_rot./R_old .* R_new;
    end
    x = sign(distortion(8)) * (x + distortion(1));
    y = sign(distortion(9)) * (y + distortion(2));
    
end