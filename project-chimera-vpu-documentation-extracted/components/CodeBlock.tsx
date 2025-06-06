
import React from 'react';

interface CodeBlockProps {
  code: string;
  language: string;
  className?: string;
}

const CodeBlock: React.FC<CodeBlockProps> = ({ code, language, className = '' }) => {
  return (
    <div className={`bg-code-bg text-code-text p-4 rounded-md shadow-md overflow-x-auto text-sm ${className}`}>
      <pre>
        <code className={`language-${language}`}>
          {code.trim()}
        </code>
      </pre>
    </div>
  );
};

export default CodeBlock;
