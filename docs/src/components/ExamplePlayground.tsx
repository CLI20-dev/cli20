import React, {useEffect, useRef, useState} from 'react';
import useBaseUrl from '@docusaurus/useBaseUrl';

function splitArgs(input: string): string[] {
  const args: string[] = [];
  let cur = '';
  let quote: '"' | "'" | null = null;
  for (const ch of input) {
    if (quote) {
      if (ch === quote) {
        quote = null;
      } else {
        cur += ch;
      }
    } else if (ch === '"' || ch === "'") {
      quote = ch;
    } else if (ch === ' ' || ch === '\t') {
      if (cur) {
        args.push(cur);
        cur = '';
      }
    } else {
      cur += ch;
    }
  }
  if (cur) args.push(cur);
  return args;
}

function SpinnerIcon() {
  return (
    <svg
      viewBox="0 0 24 24"
      width="14"
      height="14"
      fill="none"
      stroke="currentColor"
      strokeWidth="2.5"
      strokeLinecap="round"
      aria-hidden>
      <circle cx="12" cy="12" r="9" strokeOpacity="0.2" />
      <path d="M12 3a9 9 0 0 1 9 9">
        <animateTransform
          attributeName="transform"
          type="rotate"
          from="0 12 12"
          to="360 12 12"
          dur="0.7s"
          repeatCount="indefinite"
        />
      </path>
    </svg>
  );
}

type HistoryEntry = {args: string; output: string; exitCode: number};

export function ExamplePlayground({
  name,
  defaultArgs = '--help',
  setupFs,
}: {
  name: string;
  defaultArgs?: string;
  /** Optional callback to populate the Emscripten virtual FS before each run. */
  setupFs?: (fs: any) => void;
}) {
  const wasmUrl = useBaseUrl(`/wasm/${name}.js`);
  const factoryRef = useRef<Promise<(opts: object) => Promise<any>> | null>(null);
  const runningRef = useRef(false);
  const didAutoRun = useRef(false);
  const bodyRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  // Input starts empty — defaultArgs is only used for the initial auto-run.
  const [args, setArgs] = useState('');
  const [history, setHistory] = useState<HistoryEntry[]>([]);
  const [running, setRunning] = useState(false);
  const [loadError, setLoadError] = useState<string | null>(null);
  const [focused, setFocused] = useState(false);
  // Track cursor position so the block cursor renders at the right place.
  const [cursorPos, setCursorPos] = useState(0);

  const run = async (overrideArgs?: string) => {
    if (runningRef.current) return;
    runningRef.current = true;
    setRunning(true);
    setLoadError(null);

    const usedArgs = overrideArgs ?? args;
    let out = '';
    let exitCode = 0;

    try {
      if (!factoryRef.current) {
        // webpackIgnore: true — lives in static/, not bundled.
        factoryRef.current = import(/* webpackIgnore: true */ wasmUrl).then(
          (m) => m.default,
        );
      }
      const factory = await factoryRef.current;
      const mod = await factory({
        thisProgram: name,
        print: (line: string) => {
          out += line + '\n';
        },
        printErr: (line: string) => {
          out += line + '\n';
        },
        noExitRuntime: false,
      });

      // Populate the virtual FS after the module is fully initialised.
      try {
        if (setupFs) setupFs(mod.FS);
      } catch (e: unknown) {
        out += '[FS setup error: ' + String(e) + ']\n';
        exitCode = 1;
      }

      if (exitCode === 0) {
        try {
          exitCode = (mod.callMain(splitArgs(usedArgs)) as number) ?? 0;
        } catch (e: unknown) {
          const status = (e as {status?: number})?.status;
          if (typeof status === 'number') {
            exitCode = status;
          } else {
            out += '[Runtime error: ' + String(e) + ']\n';
            exitCode = 1;
          }
        }
      }

      setHistory((prev) => [
        ...prev,
        {args: usedArgs, output: out.trimEnd(), exitCode},
      ]);
    } catch (e: unknown) {
      setLoadError(String(e));
    } finally {
      runningRef.current = false;
      setRunning(false);
    }
  };

  useEffect(() => {
    if (didAutoRun.current) return;
    didAutoRun.current = true;
    void run(defaultArgs);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, []);

  // Scroll pg-body to bottom; does not affect page scroll.
  useEffect(() => {
    const el = bodyRef.current;
    if (el) el.scrollTop = el.scrollHeight;
  }, [history]);

  // Sync visual cursor position from the hidden input's selectionStart.
  const syncCursor = () => {
    requestAnimationFrame(() => {
      setCursorPos(inputRef.current?.selectionStart ?? args.length);
    });
  };

  // Move the hidden input's caret and update visual cursor.
  const moveTo = (pos: number) => {
    requestAnimationFrame(() => {
      inputRef.current?.setSelectionRange(pos, pos);
      setCursorPos(pos);
    });
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter' && !running) {
      void run();
      return;
    }

    const pos = inputRef.current?.selectionStart ?? args.length;

    if (e.ctrlKey) {
      switch (e.key) {
        case 'w': {                              // delete word before cursor
          e.preventDefault();
          let i = pos;
          while (i > 0 && args[i - 1] === ' ') i--;
          while (i > 0 && args[i - 1] !== ' ') i--;
          setArgs(args.slice(0, i) + args.slice(pos));
          moveTo(i);
          return;
        }
        case 'u': {                              // delete to beginning of line
          e.preventDefault();
          setArgs(args.slice(pos));
          moveTo(0);
          return;
        }
        case 'k': {                              // delete to end of line
          e.preventDefault();
          setArgs(args.slice(0, pos));
          moveTo(pos);
          return;
        }
        case 'a': {                              // move to beginning
          e.preventDefault();
          moveTo(0);
          return;
        }
        case 'e': {                              // move to end
          e.preventDefault();
          moveTo(args.length);
          return;
        }
      }
    }

    // Alt+Backspace — delete word before cursor (same as Ctrl+W)
    if (e.altKey && e.key === 'Backspace') {
      e.preventDefault();
      let i = pos;
      while (i > 0 && args[i - 1] === ' ') i--;
      while (i > 0 && args[i - 1] !== ' ') i--;
      setArgs(args.slice(0, i) + args.slice(pos));
      moveTo(i);
      return;
    }

    syncCursor();
  };

  const before = args.slice(0, cursorPos);
  const after = args.slice(cursorPos);
  const cursorActive = !running;

  return (
    <div className="pg-wrap">
      <div
        className="pg-terminal"
        onClick={() => inputRef.current?.focus()}>
        {/* ── title bar ─────────────────────────────────── */}
        <div className="pg-titlebar">
          <span className="pg-dot pg-dot-r" />
          <span className="pg-dot pg-dot-y" />
          <span className="pg-dot pg-dot-g" />
          <span className="pg-titlebar-label">cli20@cli20-dev: ~</span>
          <div className="pg-titlebar-spacer" />
          {running && <span className="pg-running-indicator"><SpinnerIcon /></span>}
        </div>

        {/* ── unified body: history + live input ────────── */}
        <div className="pg-body" ref={bodyRef}>
          {loadError && (
            <div className="pg-load-error">{loadError}</div>
          )}

          {history.map((entry, i) => (
            <div key={i} className="pg-entry">
              <div className="pg-cmdline">
                <span className="pg-ps1-host">cli20@cli20-dev</span>
                <span className="pg-ps1-sep">:~$</span>
                {' '}
                <span className="pg-cmd-name">{name}</span>
                {entry.args && (
                  <>{' '}<span className="pg-cmd-args">{entry.args}</span></>
                )}
                {entry.exitCode !== 0 && (
                  <span className="pg-exit-badge"> [exit {entry.exitCode}]</span>
                )}
              </div>
              {entry.output && (
                <pre className="pg-output">{entry.output}</pre>
              )}
            </div>
          ))}

          {/* active input line */}
          <div className="pg-cmdline pg-active-line">
            <span className="pg-ps1-host">cli20@cli20-dev</span>
            <span className="pg-ps1-sep">:~$</span>
            {' '}
            <span className="pg-cmd-name">{name}</span>
            {' '}
            {/* Mirror: visible text + block cursor */}
            <span className="pg-input-area" aria-hidden>
              <span className="pg-cmd-args">{before}</span>
              <span className={`pg-cursor${cursorActive ? (focused ? ' pg-cursor-blink' : ' pg-cursor-idle') : ''}`}>
                {after[0] ?? ' '}
              </span>
              <span className="pg-cmd-args">{after.slice(1)}</span>
              {!focused && <span className="pg-inline-hint">(click to focus)</span>}
            </span>
            {/* Actual input: transparent, overlaid for keyboard capture */}
            <input
              ref={inputRef}
              className="pg-input-hidden"
              value={args}
              onChange={(e) => {
                setArgs(e.target.value);
                syncCursor();
              }}
              onKeyDown={handleKeyDown}
              onKeyUp={syncCursor}
              onSelect={syncCursor}
              onFocus={() => setFocused(true)}
              onBlur={() => setFocused(false)}
              spellCheck={false}
              aria-label="Command-line arguments"
            />
          </div>
        </div>
      </div>
    </div>
  );
}
