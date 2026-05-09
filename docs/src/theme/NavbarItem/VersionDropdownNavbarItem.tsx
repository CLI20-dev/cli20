import React, {useEffect, useState, useCallback} from 'react';
import type {ReactNode} from 'react';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';

type VersionsJson = {versions: string[]};

type Props = {
  mobile?: boolean;
};

function MobileVersionDropdown({versions, prefix}: {versions: string[]; prefix: string}): ReactNode {
  const [collapsed, setCollapsed] = useState(true);
  const toggle = useCallback(() => setCollapsed((c) => !c), []);

  return (
    <li className="menu__list-item">
      <button
        className={`clean-btn menu__link menu__link--sublist menu__link--sublist-caret${collapsed ? '' : ' menu__link--active'}`}
        style={{fontSize: 'inherit'}}
        onClick={toggle}
        aria-expanded={!collapsed}>
        Versions
      </button>
      <ul className="menu__list" style={{display: collapsed ? 'none' : undefined}}>
        {versions.map((v) => (
          <li key={v} className="menu__list-item">
            <a href={`${prefix}/${v}/`} className="menu__link">
              {v}
            </a>
          </li>
        ))}
      </ul>
    </li>
  );
}

export default function VersionDropdownNavbarItem({mobile = false}: Props): ReactNode {
  const {
    siteConfig: {baseUrl},
  } = useDocusaurusContext();
  const [versions, setVersions] = useState<string[]>([]);

  useEffect(() => {
    // versions.json lives one level above the per-version subtree:
    //   baseUrl = '/cli20/nightly/'  →  repoPrefix = '/cli20'
    const segments = baseUrl.split('/').filter(Boolean);
    const versionsUrl =
      segments.length >= 1 ? `/${segments[0]}/versions.json` : '/versions.json';

    fetch(versionsUrl)
      .then((r) => r.json())
      .then((data: VersionsJson) => setVersions(data.versions))
      .catch(() => {});
  }, [baseUrl]);

  if (versions.length === 0) return null;

  const repoPrefix = baseUrl.split('/').filter(Boolean)[0] ?? '';
  const prefix = repoPrefix ? `/${repoPrefix}` : '';

  if (mobile) {
    return <MobileVersionDropdown versions={versions} prefix={prefix} />;
  }

  return (
    <div className="navbar__item dropdown dropdown--hoverable">
      <a className="navbar__link" role="button" tabIndex={0}>
        Versions
      </a>
      <ul className="dropdown__menu">
        {versions.map((v) => (
          <li key={v}>
            <a href={`${prefix}/${v}/`} className="dropdown__link">
              {v}
            </a>
          </li>
        ))}
      </ul>
    </div>
  );
}
