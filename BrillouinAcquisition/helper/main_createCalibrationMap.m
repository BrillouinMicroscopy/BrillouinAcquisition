%CREATECALIBRATIONMAP Creates a voltage-position calibration map.
% This file creates an artificial calibration map using simulated camera
% data. Can be easily enhanced providing real camera images with the
% corresponding voltage maps.

% program modes: [1, 2, 3] => [acquisition, evaluation, simulation]
acquisition = 1;
evaluation = 2;
simulation = 3;

mode = evaluation;
%% storage folder
path = 'RawData';
if ~exist(path, 'dir') && (mode == 1)
    mkdir(path);
end

filename = '_positionCalibration';

%% camera parameters
camera.imgWidth = 1280;
camera.imgHeight = 1024;
camera.pixelSize = 4.8e-6;
camera.magnification = 57;

%% Initial settings
voltages.nrSteps = 21;
voltages.ux = linspace(-0.17, 0.14, voltages.nrSteps);
voltages.uy = linspace(-0.18, 0.13, voltages.nrSteps);
[voltages.Ux, voltages.Uy] = meshgrid(voltages.ux, voltages.uy);

if mode == acquisition
    %% Initialise the NI-DAQmx to control the galvo scanners
    session = daq.createSession('ni');
    addAnalogOutputChannel(session, 'Dev1', 0:1, 'Voltage');    % voltages to control galvo scanners
    outputSingleScan(session, [0 0]);                           % set voltages to zero

    %% Camera Initialization
    vid = videoinput('pointgrey', 1, 'F7_Raw8_1280x1024_Mode0');
    vid.ROIPosition = [0 0 camera.imgWidth camera.imgHeight];
    src = getselectedsource(vid);
    src.Brightness = 0;
    src.ExposureMode = 'Manual';
    src.Exposure = 0;
    src.GainMode = 'Manual';
    src.Gain = 0;
    src.ShutterMode = 'Manual';
    src.Shutter = 0.5; 
    src.FrameRatePercentageMode = 'off';
    src.FrameRateMode = 'Manual';
    src.FrameRate = 135;
end

%% Plot voltages
figure;
subplot(2,4,1);
for jj = 1:size(voltages.Ux, 2)
    for kk = 1:size(voltages.Ux, 1)
       plot(voltages.Ux(kk,jj), voltages.Uy(kk,jj), 'color', 'red', 'marker', 'x') ;
       hold on;
    end
end
axis equal;
axis([min(voltages.Ux(:)) max(voltages.Ux(:)) min(voltages.Uy(:)) max(voltages.Uy(:))]);
xlabel('$U_1$ [V]', 'interpreter', 'latex');
ylabel('$U_2$ [V]', 'interpreter', 'latex');
title('Applied voltages');
drawnow;

%% prepare position arrays
positions.X_pix = NaN(voltages.nrSteps, voltages.nrSteps);
positions.Y_pix = NaN(voltages.nrSteps, voltages.nrSteps);

%% set voltages to galvo scanner, acquire images and extract peak position
combinedImage = zeros(camera.imgHeight, camera.imgWidth);
for jj = 1:size(voltages.Ux, 2)
    for kk = 1:size(voltages.Ux, 1)
        clc;
        disp([jj,kk])
        if mode == acquisition
            % set voltage to scanner
            outputSingleScan(session, [voltages.Ux(kk,jj) voltages.Uy(kk,jj)]);
            %% get image from camera and save it
            image = getsnapshot(vid);                                               % actual image from camera
            image = flipud(image);
            save([path filesep sprintf('image_%03.0f_%03.0f.mat', jj, kk)], 'image');
        end
        
        if mode == evaluation
            %% load image from saved file
            res = load([path filesep sprintf('image_%03.0f_%03.0f.mat', jj, kk)], 'image');
            image = res.image;
        end
        
        if mode == simulation
        %% get simulated image
            image = simCamImage(voltages.Ux(kk,jj), voltages.Uy(kk,jj), camera);    % simulated image
        end
        
        % cast image to double (for camera images)
        image = double(image);
        
        % create image showing all focus positions (just for visualization)
        combinedImage = max(cat(3, combinedImage, image), [], 3);
        
        % evaluate peak position
        [positions.X_pix(kk,jj), positions.Y_pix(kk,jj)] = evaluateCameraImage(image);
        
    end
end
% save('settings.mat', 'voltages', 'camera');
clear jj kk image;

%% convert pixel to position
positions.X_pix_centered = (positions.X_pix - camera.imgWidth/2 - 0.5);    % [�m] focus position
positions.Y_pix_centered = (positions.Y_pix - camera.imgHeight/2 - 0.5);   % [�m] focus position
positions.X_meter = camera.pixelSize/camera.magnification*(positions.X_pix - camera.imgWidth/2 - 0.5);    % [�m] focus position
positions.Y_meter = camera.pixelSize/camera.magnification*(positions.Y_pix - camera.imgHeight/2 - 0.5);   % [�m] focus position

%% extract bounds
bounds = 1e6*camera.pixelSize/camera.magnification*[-camera.imgWidth/2 camera.imgWidth/2 -camera.imgHeight/2 camera.imgHeight/2];
bounds = struct( ...
    'xMin', bounds(1), ...  % [�m] minimal x-value
    'xMax', bounds(2), ...  % [�m] maximal x-value
    'yMin', bounds(3), ...  % [�m] minimal y-value
    'yMax', bounds(4) ...   % [�m] maximal y-value
);

%% plot all focus positions
subplot(2,4,2);
imagesc(combinedImage);
set(gca, 'yDir', 'normal');
% caxis([0 100]);
axis equal;
axis([0.5 camera.imgWidth+0.5 0.5 camera.imgHeight+0.5]);
xlabel('$x$ [pix]', 'interpreter', 'latex');
ylabel('$y$ [pix]', 'interpreter', 'latex');
title('Resulting foci on camera');
drawnow;

%%
subplot(2,4,3);
hold on;
for jj = 1:size(positions.X_meter, 2)
    for kk = 1:size(positions.X_meter, 1)
        plot(1e6*positions.X_meter(kk,jj), 1e6*positions.Y_meter(kk,jj), 'color', 'red', 'marker', 'x');
    end
end
axis equal;
box on;
axis([bounds.xMin bounds.xMax bounds.yMin bounds.yMax]);
xlabel('$x$ [$\mu$m]', 'interpreter', 'latex');
ylabel('$y$ [$\mu$m]', 'interpreter', 'latex');
title('Evaluated positions');
drawnow;

%% Find distortion parameters
distortion_initial = [ ...
    16e-6, ...  % x0
    -8e-6, ...  % y0
    -0.35, ...  % rho
    5e-4, ...   % d
    1.1e-4, ... % c
    5.1e-5, ... % b
    2.5e-4, ... % a
    1, ...      % fliplr
    -1, ...     % flipud
];

model = @(Ux, Uy, distortion) VoltageToPosition(Ux, Uy, distortion);

% optimize analytical calibration
errorFuncDist = @(distortion) errorFunctionDistortion(model, distortion, voltages, positions);

options = optimset('MaxFunEvals', 1000000, 'MaxIter', 1000000, 'TolFun', 1e-6, 'TolX', 1e-6);
distortion = fminsearch(errorFuncDist, distortion_initial, options);

%% Check if found parameters work well
positions_desired.nrStepsX = 21;
positions_desired.nrStepsY = 16;
positions_desired.x_meter = 1e-6*linspace(-50, 50, positions_desired.nrStepsX);
positions_desired.y_meter = 1e-6*linspace(-40, 40, positions_desired.nrStepsY);
[positions_desired.X_meter, positions_desired.Y_meter] = meshgrid(positions_desired.x_meter, positions_desired.y_meter);

positions_desired.X_pix_centered = positions_desired.X_meter*camera.magnification/camera.pixelSize;
positions_desired.Y_pix_centered = positions_desired.Y_meter*camera.magnification/camera.pixelSize;

% find required voltages
[voltages_required.Ux, voltages_required.Uy] = PositionToVoltage(positions_desired.X_meter, positions_desired.Y_meter, distortion);

%% Plot voltages
subplot(2,4,5);
for jj = 1:size(voltages_required.Ux, 2)
    for kk = 1:size(voltages_required.Ux, 1)
       plot(voltages_required.Ux(kk,jj), voltages_required.Uy(kk,jj), 'color', 'red', 'marker', 'x') ;
       hold on;
    end
end
axis equal;
axis([min(voltages_required.Ux(:)) max(voltages_required.Ux(:)) min(voltages_required.Uy(:)) max(voltages_required.Uy(:))]);
xlabel('$U_1$ [V]', 'interpreter', 'latex');
ylabel('$U_2$ [V]', 'interpreter', 'latex');
title('Applied voltages');
drawnow;

%% prepare position arrays
positions_resulting.X_pix = NaN(positions_desired.nrStepsY, positions_desired.nrStepsX);
positions_resulting.Y_pix = NaN(positions_desired.nrStepsY, positions_desired.nrStepsX);

% set voltages and acquire image
combinedControlImage = zeros(camera.imgHeight, camera.imgWidth);
for jj = 1:size(voltages_required.Ux, 2)
    for kk = 1:size(voltages_required.Ux, 1)
        clc;
        disp([jj,kk]);
        if mode == acquisition
            % set voltage to scanner
            outputSingleScan(session, [voltages_required.Ux(kk,jj) voltages_required.Uy(kk,jj)]);
            %% get image from camera and save it
            image = getsnapshot(vid);                                               % actual image from camera
            image = flipud(image);
            save([path filesep sprintf('controlimage_%03.0f_%03.0f.mat', jj, kk)], 'image');
        end
        
        if mode == evaluation
            %% load image from saved file
            res = load([path filesep sprintf('controlimage_%03.0f_%03.0f.mat', jj, kk)], 'image');
            image = res.image;
        end
        
        if mode == simulation
        %% get simulated image
            image = simCamImage(voltages_required.Ux(kk,jj), voltages_required.Uy(kk,jj), camera);    % simulated image
        end
        
        % cast image to double (for camera images)
        image = double(image);
        
        % create image showing all focus positions (just for visualization)
        combinedControlImage = max(cat(3, combinedControlImage, image), [], 3);
        
        
        %% evaluate peak position
        [positions_resulting.X_pix(kk,jj), positions_resulting.Y_pix(kk,jj)] = evaluateCameraImage(image);
    end
end
save('settings_fitted.mat', 'positions_desired', 'voltages_required');
clear jj kk image;

%% convert pixel to position
positions_resulting.X_pix_centered = (positions_resulting.X_pix - camera.imgWidth/2 - 0.5);    % [pix] focus position
positions_resulting.Y_pix_centered = (positions_resulting.Y_pix - camera.imgHeight/2 - 0.5);   % [pix] focus position
positions_resulting.X_meter = camera.pixelSize/camera.magnification*(positions_resulting.X_pix - camera.imgWidth/2 - 0.5);    % [pix] focus position
positions_resulting.Y_meter = camera.pixelSize/camera.magnification*(positions_resulting.Y_pix - camera.imgHeight/2 - 0.5);   % [pix] focus position

%% plot all focus positions
subplot(2,4,6);
imagesc(combinedControlImage);
set(gca, 'yDir', 'normal');
caxis([0 100]);
axis equal;
axis([0.5 camera.imgWidth+0.5 0.5 camera.imgHeight+0.5]);
xlabel('$x$ [pix]', 'interpreter', 'latex');
ylabel('$y$ [pix]', 'interpreter', 'latex');
title('Resulting foci on camera');
drawnow;

%%
subplot(2,4,7);
hold on;
for jj = 1:size(positions_resulting.X_meter, 2)
    for kk = 1:size(positions_resulting.X_meter, 1)
        plot(1e6*positions_resulting.X_meter(kk,jj), 1e6*positions_resulting.Y_meter(kk,jj), 'color', 'red', 'marker', 'x');
    end
end
axis equal;
box on;
axis([bounds.xMin bounds.xMax bounds.yMin bounds.yMax]);
xlabel('$x$ [$\mu$m]', 'interpreter', 'latex');
ylabel('$y$ [$\mu$m]', 'interpreter', 'latex');
title('Evaluated positions');
drawnow;

%% calculate error
distances = sqrt((positions_resulting.X_meter - positions_desired.X_meter).^2 + (positions_resulting.Y_meter - positions_desired.Y_meter).^2);
subplot(2,4,8);
imagesc(1e6*positions_resulting.X_meter(1,:), 1e6*positions_resulting.Y_meter(:,1), 1e9*distances);
axis equal;
set(gca, 'yDir', 'normal');
axis([bounds.xMin bounds.xMax bounds.yMin bounds.yMax]);
xlabel('$x$ [$\mu$m]', 'interpreter', 'latex');
ylabel('$y$ [$\mu$m]', 'interpreter', 'latex');
title('Error');
cb = colorbar;
title(cb, '[nm]', 'interpreter', 'latex');

%% write calibration to file
% data to write:
%
% date
% translation ->
%   x
%   y
% rho
% coefficients ->
%   d
%   c
%   b
%   a
%   lr
%   ud
% bounds ->
%   xMin
%   xMax
%   yMin
%   yMax

% create filename
date = datetime('now', 'InputFormat', 'uuuu-MM-dd''T''HH:mm:ss.SSSXXX', 'TimeZone', 'UTC');
date.TimeZone = 'local';
date.Format = 'uuuu-MM-dd''T''HHmmssX';
shortdate = char(date);

fullFilename = ['../' filename '_' shortdate '.h5'];

% create file
fid = fopen(fullFilename,'w');
fclose(fid);

%% write date
date.Format = 'uuuu-MM-dd''T''HH:mm:ss.SSSXXX';
fulldate = char(date);
h5writeatt(fullFilename, '/', 'date', fulldate);

%% write translation
d = '/translation/';

ds = [d 'x'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(1));

ds = [d 'y'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(2));

%% write rotation
ds = '/rotation';
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(3));

%% write coefficients
d = '/coefficients/';

ds = [d 'a'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(7));

ds = [d 'b'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(6));

ds = [d 'c'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(5));

ds = [d 'd'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(4));

ds = [d 'lr'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(8));

ds = [d 'ud'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, distortion(9));

% write bounds
d = '/bounds/';

ds = [d 'xMin'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, bounds.xMin);

ds = [d 'xMax'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, bounds.xMax);

ds = [d 'yMin'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, bounds.yMin);

ds = [d 'yMax'];
h5create(fullFilename, ds, 1, 'ChunkSize', 1);
h5write(fullFilename, ds, bounds.yMax);
