import React, {useEffect, useState} from 'react';
import type {ReactNode} from 'react';
import useDocusaurusContext from '@docusaurus/useDocusaurusContext';

type VersionsJson = {versions: string[]};

export default function VersionDropdownNavbarItem(): ReactNode {
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

  const repoPrefix =
    baseUrl.split('/').filter(Boolean)[0] ?? '';
  const prefix = repoPrefix ? `/${repoPrefix}` : '';

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
