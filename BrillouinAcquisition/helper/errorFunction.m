function [error, fitted] = errorFunction(model, params, X, Y, image)
    fitted = model(X, Y, params);
                
    errorVector = fitted - image;
    
    tmp = errorVector.^2;
    error = sum(tmp(:));
end