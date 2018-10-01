function [Ux, Uy] = PositionToVoltage(x, y, distortion)

    x = (x / distortion(8) - distortion(1));
    y = (y / distortion(9) - distortion(2));
    
    rho = -1*distortion(3);
    
    x_rot = x * cos(rho) - y * sin(rho);
    y_rot = x * sin(rho) + y * cos(rho);

    R_old = sqrt(x.^2 + y.^2);
    
    % solve for R_new
    poly.a = distortion(7);
    poly.b = distortion(6);
    poly.c = distortion(5);
    poly.d = distortion(4);
    R_new = NaN(size(R_old));
    for jj = 1:size(R_old,1)
        for kk = 1:size(R_old,2)
            poly.e = -1*R_old(jj,kk);
            R_new(jj,kk) = solveQuartic(poly);
        end
    end
    
    if (abs(R_old) < 1e-12)
        Ux = x_rot;
        Uy = y_rot;
    else
        Ux = x_rot./R_old .* R_new;
        Uy = y_rot./R_old .* R_new;
    end    
end