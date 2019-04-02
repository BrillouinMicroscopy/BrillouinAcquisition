function [x, y] = VoltageToPosition(Ux, Uy, x_cal, y_cal, Ux_cal, Uy_cal)

    idxgood = ~(isnan(x_cal) | isnan(y_cal) | isnan(Ux_cal) | isnan(Uy_cal));

    x = griddata(Ux_cal(idxgood), Uy_cal(idxgood), x_cal(idxgood), Ux, Uy, 'v4');
    y = griddata(Ux_cal(idxgood), Uy_cal(idxgood), y_cal(idxgood), Ux, Uy, 'v4');

end