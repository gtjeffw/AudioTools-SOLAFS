# Notes on Dan Ellis's `solafs.m`

The C++ SOLAFS in `../SOLAFS.cpp` was ported from Dan Ellis's MATLAB implementation, version 1.3 (2006-04-09):

- Code: <https://www.ee.columbia.edu/~dpwe/LabROSA/matlab/solafs.m>
- Page: <https://www.ee.columbia.edu/~dpwe/LabROSA/matlab/solafs-matlab.html>

It is not copied into this repository (it carries no license). These notes record the changes made to it while
porting, which the C++ keeps.

## Changes made to the MATLAB version

1. **Shift prediction.** The original predicts the next skew as `km + ((xpos - lastxpos) - (ypos - lastypos))`,
   but then sets `lastypos = xpos` rather than `ypos`, so after the first step the prediction drifts by
   `xpos - ypos` and falls outside `[0, Kmax]` unless the speed ratio is close to 1 (the code then searches every
   step). It was replaced with the prediction from Hejna & Musicus eq. (4) (patent US 5,175,769, eqn. 13), with a
   constant step difference:

   ```matlab
   Sa = floor(F*Ss);  Sdiff = Ss - Sa;
   kmpred = km + Sdiff;    % TODO: should probably take max of 0 or (km+Sdiff)
   ```

   Prediction is only attempted when `abs(Sdiff) <= Kmax`, since otherwise it can never land in range.
2. **No cross-fade after a successful prediction.** When the predicted skew is used, the input and the output
   overlap are identical, so the paper notes the averaging is unnecessary; the cross-fade is applied only after a
   search (a `didSearch` flag).
3. **Integer analysis position.** `xpos = floor(F * ypos)` instead of `F * ypos`.
4. **Search range.** The original has a commented-out restriction `Kmin = max(0, xabs - xpos)`, the paper's
   suggestion (section 5) that each window start no earlier than the previous one. It stays disabled: in testing,
   restricting the search this way pinned the skew against `Kmax` when stretching (expansion), because prediction
   raises the skew by `Ss - Sa` each step while the restricted search can only lower it by `Sa`.

The C++ also differs from the MATLAB in ways that came later, while comparing against the paper; see the
git history of `SOLAFS.cpp`.
