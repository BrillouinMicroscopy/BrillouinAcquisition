function [posX, posY] = evaluateCameraImage(image)
%EVALUATECAMERAIMAGE
% This script fits a 2D Gaussian to an image and return the found x- and y-
% coordinates. Assumes that only one peak is present.
    
    %% find maximum value
    [I, ind] = max(image(:));
    [indY, indX] = ind2sub(size(image), ind);
    
    
    %% discard values if peak is to small
    if (I < 50)
        posX = NaN;
        posY = NaN;
        return
    end

    %% only select a fraction of the image size around maximum to speed up gaussian fitting
    ymin = indY - round(size(image, 1)/20);
    if (ymin < 1); ymin = 1; end
    ymax = indY + round(size(image, 1)/20);
    if (ymax > size(image, 1)); ymax = size(image, 1); end
    xmin = indX - round(size(image, 2)/20);
    if (xmin < 1); xmin = 1; end
    xmax = indX + round(size(image, 2)/20);
    if (xmax > size(image, 2)); xmax = size(image, 2); end
    
    detail = image(ymin:ymax, xmin:xmax);
    
    %% find maximum value for detail
    [I, ind] = max(detail(:));
    [indY, indX] = ind2sub(size(detail), ind);
    
%     figure(142);
%     imagesc(detail);
%     drawnow;

    x = 1:size(detail, 2);
    y = 1:size(detail, 1);
    [X, Y] = meshgrid(x, y);
    
    start = [I, indX, indY, 10];
    model = @(X, Y, params) params(1) * exp(-((X-params(2)).^2 + (Y-params(3)).^2) / params(4));
    
    % optimize analytical calibration
    errorFunc = @(params) errorFunction(model, params, X, Y, detail);

    options = optimset('MaxFunEvals', 10000, 'MaxIter', 1000, 'TolFun', 1e-2, 'TolX', 1e-2);
    params = fminsearch(errorFunc, start, options);
    
    % discard values if peak is to small
    if params(1) < 10
        posX = NaN;
        posY = NaN;
    else
        posX = params(2) + xmin - 1;
        posY = params(3) + ymin - 1;
    end
    
    % discard values if they are outside of the image
    if (posX > size(image, 2) || posX < 1 || posY > size(image, 1) || posY < 1)
        posX = NaN;
        posY = NaN;
    end
    
end