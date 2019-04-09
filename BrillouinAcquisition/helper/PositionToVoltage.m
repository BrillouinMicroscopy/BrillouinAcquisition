function [Ux, Uy] = PositionToVoltage(x, y, x_cal, y_cal, Ux_cal, Uy_cal)

    idxgood = ~(isnan(x_cal) | isnan(y_cal) | isnan(Ux_cal) | isnan(Uy_cal));

    Ux = griddata(x_cal(idxgood), y_cal(idxgood), Ux_cal(idxgood), x, y, 'v4');
    Uy = griddata(x_cal(idxgood), y_cal(idxgood), Uy_cal(idxgood), x, y, 'v4');

end