function [image] = simCamImage(voltageX, voltageY, camera)
    spotSize = 10;  % [pix] size of the laser focus
    distortion = [ ...
        200, ...    % x0
        100, ...    % y0
        0.35, ...   % rho
        3000, ...   % d
        1300, ...   % c
        600, ...    % b
        3000, ...   % a
    ];
    
    [posX, posY] = VoltageToPosition(voltageX, voltageY, distortion);

    x = linspace(-camera.imgWidth/2, camera.imgWidth/2, camera.imgWidth);
    y = linspace(-camera.imgHeight/2, camera.imgHeight/2, camera.imgHeight);
    [X, Y] = meshgrid(x, y);
    
    image = 100*exp(-((X-posX).^2 + (Y-posY).^2) / spotSize);
    
    %% debug plot
%     figure(123);
%     imagesc(x, y, image);
%     axis equal;
%     xlim([min(x(:)), max(x(:))]);
%     ylim([min(y(:)), max(y(:))]);

end