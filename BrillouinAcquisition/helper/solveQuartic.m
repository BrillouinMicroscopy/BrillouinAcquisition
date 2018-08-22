function [x] = solveQuartic(poly)
    % shorthands for the variables
    a = poly.a;
    b = poly.b;
    c = poly.c;
    d = poly.d;
    e = poly.e;
    
    %% easy solution using matlab routines
%     p = [a b c d e];
%     % find all four roots
%     x = roots(p);
    
    %% hard solution by foot, can be used similarly e.g. in C++
    % intermediate variables
    p1 = 2*c^3 - 9 *b*c*d + 27*a*d^2 + 27*b^2*e - 72*a*c*e;
    p2 = p1 + sqrt(-4*(c^2 - 3*b*d + 12*a*e)^3 + p1^2);
    p3 = (c^2 - 3*b*d + 12*a*e) / (3*a*(p2/2).^(1/3)) + (p2/2).^(1/3) / (3*a);
    
    p4 = sqrt(b^2 / (4*a^2) - (2*c) / (3*a) + p3);
    p5 = b^2 / (2*a^2) - (4*c) / (3*a) - p3;
    p6 = (-1*(b^3) / a^3 + (4*b*c) / a^2 - (8*d)/a) / (4*p4);
    
    x(1) = -b/(4*a) - p4/2 - sqrt(p5 - p6)/2;
    x(2) = -b/(4*a) - p4/2 + sqrt(p5 - p6)/2;
    x(3) = -b/(4*a) + p4/2 - sqrt(p5 + p6)/2;
    x(4) = -b/(4*a) + p4/2 + sqrt(p5 + p6)/2;
    
    %% select only the real, positive solutions
    % select only the real ones
    x = x(abs(imag(x)) < 1e-10);
    x = real(x);
    % select only the positive one
    x = x(x>0);
    if isempty(x)
        x = NaN;
    end
    x = min(x);
end