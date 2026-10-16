import { useEffect, useId, useState } from "react";

interface Props {
  label: string;
  value: number;
  onChange: (v: number) => void;
  step?: number;
  min?: number;
  max?: number;
  unit?: string;
  hint?: string;
  disabled?: boolean;
}

/** A number input that lets you type freely and only reports valid numbers. */
export function NumberField({ label, value, onChange, step = 1, min, max, unit, hint, disabled }: Props) {
  const id = useId();
  const [text, setText] = useState(String(value));
  useEffect(() => {
    if (Number(text) !== value) setText(String(value));
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [value]);
  return (
    <label className="field" htmlFor={id}>
      <span className="field-label">{label}</span>
      <span className="field-input">
        <input
          id={id}
          type="number"
          inputMode="decimal"
          value={text}
          step={step}
          min={min}
          max={max}
          disabled={disabled}
          onChange={(e) => {
            setText(e.target.value);
            const v = e.target.valueAsNumber;
            if (Number.isFinite(v)) onChange(v);
          }}
          onBlur={() => setText(String(value))}
        />
        {unit && <span className="unit">{unit}</span>}
      </span>
      {hint && <span className="field-hint">{hint}</span>}
    </label>
  );
}
