import React, {useMemo, useState} from 'react';
import results from '@site/src/data/benchmarks/results.json';

const caseLabels = {
  simple: 'Simple',
  many_options: 'Many options',
  subcommand: 'Subcommand',
};

const caseColors = {
  simple: '#2563eb',
  many_options: '#b45309',
  subcommand: '#0f766e',
};

const caseDash = {
  simple: '',
  many_options: '7 4',
  subcommand: '2 4',
};

const libraryLabels = {
  cli20: 'cli20',
  cli11: 'CLI11',
  argparse: 'argparse',
  cxxopts: 'cxxopts',
  boost: 'Boost*',
};

function number(value) {
  return new Intl.NumberFormat('en', {
    maximumFractionDigits: value < 10 ? 2 : 1,
  }).format(value);
}

function bytes(value) {
  if (value >= 1024 * 1024) return `${number(value / (1024 * 1024))} MiB`;
  if (value >= 1024) return `${number(value / 1024)} KiB`;
  return `${number(value)} B`;
}

function metricValue(row, metric) {
  if (metric === 'runtime') return row.timeNs;
  if (metric === 'allocations') return row.allocations;
  if (metric === 'allocatedBytes') return row.allocatedBytes;
  if (metric === 'binarySize') return row.strippedBytes;
  if (metric === 'compileTime') return row.seconds;
  return 0;
}

function formatMetric(value, metric) {
  if (metric === 'runtime') return `${number(value)} ns`;
  if (metric === 'allocations') return `${number(value)} allocs`;
  if (metric === 'allocatedBytes' || metric === 'binarySize') return bytes(value);
  if (metric === 'compileTime') return `${number(value)} s`;
  return number(value);
}

function rowsFor(metric) {
  if (metric === 'binarySize') return results.size;
  if (metric === 'compileTime') return results.compileTime;
  return results.runtime;
}

function LineChart({title, metric, standard}) {
  const rows = rowsFor(metric).filter((row) => row.standard === standard);
  const max = Math.max(...rows.map((row) => metricValue(row, metric)), 1);
  const width = 760;
  const height = 320;
  const padding = {top: 18, right: 28, bottom: 68, left: 82};
  const plotWidth = width - padding.left - padding.right;
  const plotHeight = height - padding.top - padding.bottom;
  const xFor = (index) =>
    padding.left + (plotWidth * index) / Math.max(results.libraries.length - 1, 1);
  const yFor = (value) => padding.top + plotHeight - (value / max) * plotHeight;
  const ticks = [0, 0.25, 0.5, 0.75, 1];

  return (
    <section className="bench-card">
      <div className="bench-card-heading">
        <h3>{title}</h3>
        <span>smaller is better</span>
      </div>
      <div className="bench-line-chart" role="img" aria-label={`${title} benchmark chart`}>
        <svg viewBox={`0 0 ${width} ${height}`}>
          {ticks.map((tick) => {
            const value = max * tick;
            const y = yFor(value);
            return (
              <g key={tick}>
                <line className="bench-grid" x1={padding.left} x2={width - padding.right} y1={y} y2={y} />
                <text className="bench-axis-label" x={padding.left - 10} y={y + 4} textAnchor="end">
                  {formatMetric(value, metric)}
                </text>
              </g>
            );
          })}
          <line
            className="bench-axis"
            x1={padding.left}
            x2={width - padding.right}
            y1={padding.top + plotHeight}
            y2={padding.top + plotHeight}
          />
          <line
            className="bench-axis"
            x1={padding.left}
            x2={padding.left}
            y1={padding.top}
            y2={padding.top + plotHeight}
          />
          {results.libraries.map((library, index) => (
            <g key={library}>
              <line
                className="bench-x-tick"
                x1={xFor(index)}
                x2={xFor(index)}
                y1={padding.top + plotHeight}
                y2={padding.top + plotHeight + 6}
              />
              <text
                className="bench-x-label"
                x={xFor(index)}
                y={padding.top + plotHeight + 24}
                textAnchor="middle">
                {libraryLabels[library] ?? library}
              </text>
            </g>
          ))}
          {results.cases.map((caseName) => {
            const points = results.libraries
              .map((library, index) => {
                const row = rows.find((item) => item.case === caseName && item.library === library);
                if (!row) return null;
                return {
                  x: xFor(index),
                  y: yFor(metricValue(row, metric)),
                  value: metricValue(row, metric),
                  library,
                };
              })
              .filter(Boolean);
            const path = points.map((point, index) => `${index === 0 ? 'M' : 'L'} ${point.x} ${point.y}`).join(' ');
            return (
              <g key={caseName}>
                <path
                  className="bench-line"
                  d={path}
                  fill="none"
                  stroke={caseColors[caseName] ?? '#334155'}
                  strokeDasharray={caseDash[caseName]}
                />
                {points.map((point) => (
                  <g key={`${caseName}-${point.library}`}>
                    <circle
                      className="bench-point"
                      cx={point.x}
                      cy={point.y}
                      fill={caseColors[caseName] ?? '#334155'}
                      r="4"
                    />
                    <title>
                      {`${caseLabels[caseName] ?? caseName} / ${
                        libraryLabels[point.library] ?? point.library
                      }: ${formatMetric(point.value, metric)}`}
                    </title>
                  </g>
                ))}
              </g>
            );
          })}
        </svg>
        <div className="bench-legend">
          {results.cases.map((caseName) => (
            <span className="bench-legend-item" key={caseName}>
              <span
                className="bench-legend-line"
                style={{
                  borderColor: caseColors[caseName] ?? '#334155',
                  borderTopStyle: caseName === 'simple' ? 'solid' : caseName === 'many_options' ? 'dashed' : 'dotted',
                }}
              />
              {caseLabels[caseName] ?? caseName}
            </span>
          ))}
        </div>
      </div>
    </section>
  );
}

function ResultTable({standard}) {
  const rows = useMemo(() => {
    return results.runtime
      .filter((row) => row.standard === standard)
      .map((runtimeRow) => {
        const sizeRow = results.size.find(
          (row) =>
            row.standard === standard &&
            row.case === runtimeRow.case &&
            row.library === runtimeRow.library,
        );
        const compileRow = results.compileTime.find(
          (row) =>
            row.standard === standard &&
            row.case === runtimeRow.case &&
            row.library === runtimeRow.library,
        );
        return {runtimeRow, sizeRow, compileRow};
      });
  }, [standard]);

  return (
    <div className="bench-table-wrap">
      <table className="bench-table">
        <thead>
          <tr>
            <th>Case</th>
            <th>Library</th>
            <th>Runtime</th>
            <th>Allocs</th>
            <th>Bytes</th>
            <th>Stripped size</th>
            <th>Compile time</th>
          </tr>
        </thead>
        <tbody>
          {rows.map(({runtimeRow, sizeRow, compileRow}) => (
            <tr key={`${runtimeRow.case}-${runtimeRow.library}`}>
              <td>{caseLabels[runtimeRow.case] ?? runtimeRow.case}</td>
              <td>{libraryLabels[runtimeRow.library] ?? runtimeRow.library}</td>
              <td>{formatMetric(runtimeRow.timeNs, 'runtime')}</td>
              <td>{formatMetric(runtimeRow.allocations, 'allocations')}</td>
              <td>{formatMetric(runtimeRow.allocatedBytes, 'allocatedBytes')}</td>
              <td>{sizeRow ? formatMetric(sizeRow.strippedBytes, 'binarySize') : '-'}</td>
              <td>{compileRow ? formatMetric(compileRow.seconds, 'compileTime') : '-'}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}

export default function BenchmarkCharts() {
  const standards = results.standards.length > 0 ? results.standards : ['cxx20'];
  const [standard, setStandard] = useState(standards[0]);

  if (results.runtime.length === 0) {
    return (
      <div className="bench-empty">
        Benchmark data has not been generated yet.
      </div>
    );
  }

  return (
    <div className="bench-results">
      <div className="bench-toolbar">
        <span>Language mode</span>
        {standards.map((item) => (
          <button
            className={item === standard ? 'bench-tab bench-tab-active' : 'bench-tab'}
            key={item}
            onClick={() => setStandard(item)}
            type="button">
            {item.replace('cxx', 'C++')}
          </button>
        ))}
      </div>
      <LineChart title="Runtime" metric="runtime" standard={standard} />
      <LineChart title="Allocation count" metric="allocations" standard={standard} />
      <LineChart title="Runtime allocated bytes" metric="allocatedBytes" standard={standard} />
      <LineChart title="Stripped binary size" metric="binarySize" standard={standard} />
      <LineChart title="Compile time" metric="compileTime" standard={standard} />
      <p className="bench-note">
        * Boost.Program_options is a reference value: it links against the
        Boost.Program_options shared library, so binary-size results do not
        represent a fully self-contained executable in the same way as the
        header-oriented libraries.
      </p>
      <ResultTable standard={standard} />
    </div>
  );
}
