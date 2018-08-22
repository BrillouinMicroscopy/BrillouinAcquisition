function [error, positionsFit] = errorFunctionDistortion(model, distortion, voltages, positions)
    [positionsFit.X, positionsFit.Y] = model(voltages.Ux, voltages.Uy, distortion);
                
    distances = sqrt((positionsFit.X - positions.X_meter).^2 + (positionsFit.Y - positions.Y_meter).^2);
    
    error = nansum(distances(:));
end